// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>
#include "list.h"

#define FILE_CLOSE_FROM_KERNEL		0x00
#define FILE_OPEN_FROM_KERNEL		0x01
#define FILE_CLOSE_READY		0x02

#define	FILE_READ_END			0x04
#define FILE_READ_ERR			0x08
#define	FILE_READ_TO_FIFO_ERR		0x10

#define	FILE_WRITE_GET_FIFO_ERR		0x20
#define	FILE_WRITE_ERR			0x40

#define DEBUG_FILE_NAME_LEN 200

struct debug_file_param {
	int id;
	int status;
	int flags;
	unsigned int mode;
	char filename[DEBUG_FILE_NAME_LEN];
};

struct debug_file {
	struct debug_file *filp;
	int fd_write;
	int fd_read;
	pthread_t thread;
	struct list_head list;
	struct debug_file_param param;
};

static pthread_spinlock_t spinlock;
static int files_info_fd;
static int page_size;
//static const char *debug_path = "/data/debug/";
static const char *debug_path = "/tmp/data/";
static const char *proc_path = "/proc/debug_file/";
static LIST_HEAD(files_list);
static LIST_HEAD(files_release_list);

static void *file_read_write_thread_func(void *arg)
{
	int ret, count, len;
	struct debug_file *filp = arg;
	char full_name[DEBUG_FILE_NAME_LEN + 20];
	char *page_buf;
	struct debug_file_param param;
	int status = FILE_OPEN_FROM_KERNEL;
	fd_set fd_set_read;
	struct timeval time;

	FD_ZERO(&fd_set_read);
	time.tv_sec = 0;
	time.tv_usec = 10000;

	strcpy(full_name, proc_path);
	strcat(full_name, basename(filp->param.filename));
	page_buf = malloc(page_size);

	if (filp->param.flags == O_RDONLY) {
		filp->fd_read = open(filp->param.filename, O_RDONLY);
		printf("app open file: %s, id: %d, ready read\n",
				filp->param.filename, filp->param.id);
		filp->fd_write = open(full_name, O_WRONLY);

		while (1) {
			FD_SET(filp->fd_read, &fd_set_read);
			ret = select(filp->fd_read + 1, &fd_set_read, NULL, NULL, &time);
			if (ret < 0)
				printf("app %s select failed\n", filp->param.filename);
			if (FD_ISSET(filp->fd_read, &fd_set_read)) {
				count = read(filp->fd_read, page_buf, page_size);
				if (count < 0) {
					status = FILE_READ_ERR;
					printf("app file %s read error: %s\n",
						filp->param.filename, strerror(errno));
					goto read_exit;
				}
				if (count == 0) {
					printf("app file %s read end\n", filp->param.filename);
					status = FILE_READ_END;
					goto read_exit;
				}
				len = 0;
				while (len < count) {
					ret = write(filp->fd_write, page_buf, count - len);
					if (ret < 0) {
						status = FILE_READ_TO_FIFO_ERR;
						goto read_exit;
					}
					len += ret;
					if (ret == 0)
						usleep(10000);
				}
			}
			if (filp->param.status == FILE_CLOSE_FROM_KERNEL) {
				status = FILE_CLOSE_READY;
				printf("app receive close read file %s\n", full_name);
				goto read_exit;
			}
		}
read_exit:
		memcpy(&param, &filp->param, sizeof(struct debug_file_param));
		if (filp->param.status == FILE_CLOSE_FROM_KERNEL)
			param.status = FILE_CLOSE_READY;
		else
			param.status = status;
		len = 0;
		while (len < sizeof(struct debug_file_param)) {
			ret = write(files_info_fd, &param + len,
				    sizeof(struct debug_file_param) - len);
			if (ret < 0) {
				printf("app write proc files_info failed: %s\n", strerror(errno));
				goto exit;
			}
			len += ret;
			if (ret == 0)
				usleep(10000);
		}
	} else if (filp->param.flags & O_WRONLY) {
		filp->fd_read = open(full_name, O_RDONLY);
		strcpy(full_name, filp->param.filename);
		filp->fd_write = open(full_name, filp->param.flags);
		printf("app open file: %s, id: %d, ready write\n",
				filp->param.filename, filp->param.id);

		while (1) {
			FD_SET(filp->fd_read, &fd_set_read);
			ret = select(filp->fd_read + 1, &fd_set_read, NULL, NULL, &time);
			if (ret < 0)
				printf("app %s select failed\n", filp->param.filename);
			if (FD_ISSET(filp->fd_read, &fd_set_read)) {
				count = read(filp->fd_read, page_buf, page_size);
				if (count < 0) {
					status = FILE_WRITE_GET_FIFO_ERR;
					printf("app file %s%s read failed: %s\n", proc_path,
						filp->param.filename, strerror(errno));
					goto write_exit;
				}
				len = 0;
				while (len < count) {
					ret = write(filp->fd_write, page_buf, count - len);
					if (ret < 0) {
						status = FILE_WRITE_ERR;
						printf("app file %s write failed %s\n",
								full_name, strerror(errno));
						goto write_exit;
					}
					len += ret;
				}
			}
			if (filp->param.status == FILE_CLOSE_FROM_KERNEL) {
				status = FILE_CLOSE_READY;
				printf("app receive close write file %s\n", full_name);
				goto write_exit;
			}
		}

write_exit:
		memcpy(&param, &filp->param, sizeof(struct debug_file_param));
		if (filp->param.status == FILE_CLOSE_FROM_KERNEL)
			param.status = FILE_CLOSE_READY;
		else
			param.status = status;
		len = 0;
		while (len < sizeof(struct debug_file_param)) {
			ret = write(files_info_fd, &param + len,
				    sizeof(struct debug_file_param) - len);
			if (ret < 0) {
				printf("app write proc files_info failed: %s\n", strerror(errno));
				goto exit;
			}
			len += ret;
			if (ret == 0)
				usleep(10000);
		}
	}

exit:
	close(filp->fd_read);
	close(filp->fd_write);
	free(page_buf);

	return NULL;
}

static void *files_info_thread_func(void *arg)
{
	int ret;
	struct debug_file *filp;
	struct debug_file *entry;
	int len = 0;
	void *res;
	fd_set fd_set_read;

	files_info_fd = open("/proc/debug_file/files_info", O_RDWR);
	if (files_info_fd < 0) {
		printf("open /proc/debug_file/files_info failed: %s\n", strerror(errno));
		return NULL;
	}

	FD_ZERO(&fd_set_read);

	while (1) {
		if (!len) {
			filp = malloc(sizeof(struct debug_file));
			filp->filp = filp;
		}
		FD_SET(files_info_fd, &fd_set_read);
		if (select(files_info_fd + 1, &fd_set_read, NULL, NULL, NULL) < 0)
			printf("/proc/debug_file/files_info select failed\n");
		ret = read(files_info_fd, &filp->param + len,
				sizeof(struct debug_file_param) - len);
		if (ret < 0) {
			printf("read proc files_info failed: %s, read %d bytes\n",
				strerror(errno), len);
			free(filp);
			exit(EXIT_FAILURE);
		}
		len += ret;
		if (len < sizeof(struct debug_file_param)) {
			continue;
		} else if (len > sizeof(struct debug_file_param)) {
			len = 0;
			free(filp);
			continue;
		} else if (len == sizeof(struct debug_file_param)) {
			len = 0;
		}

		if (filp->param.status == FILE_OPEN_FROM_KERNEL) {
			list_add(&filp->list, &files_list);
			ret = pthread_create(&filp->thread, NULL,
					     file_read_write_thread_func,
					     filp);
			if (ret)
				printf("create debug file(%s) thread failed: %s\n",
					filp->param.filename, strerror(errno));
		} else {
			list_for_each_entry(entry, &files_list, list) {
				if (filp->param.id == entry->param.id)
					break;
			}
			if (&entry->list != &files_list) {
				if (filp->param.status == FILE_CLOSE_FROM_KERNEL) {
					entry->param.status = FILE_CLOSE_FROM_KERNEL;
				} else {
					pthread_join(entry->thread, &res);
					free(res);
					list_del(&entry->list);
					free(entry);
				}
			} else {
				printf("files_list not find the file %s\n", filp->param.filename);
			}
			free(filp);
		}
	}
	close(files_info_fd);
	return NULL;
}

int main(int argc, char *argv[])
{
	int ret;
	void *res;
	pthread_t files_info_thread;
	char path[20];

	page_size = getpagesize();

	if (argc > 1)
		sprintf(path, "%s", argv[1]);
	else
		sprintf(path, "%s", debug_path);

	if (!opendir(path)) {
		printf("open %s failed: %s\n", path, strerror(errno));
		ret = mkdir(path, 0666);
		if (ret) {
			printf("mkdir %s failed: %s\n", path, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);

	ret = pthread_create(&files_info_thread, NULL, files_info_thread_func, NULL);
	if (ret) {
		printf("create files_info_thread failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ret = pthread_join(files_info_thread, &res);
	if (ret) {
		printf("thread_join files_info_thread failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	free(res);

	pthread_spin_destroy(&spinlock);

	exit(EXIT_SUCCESS);
}
