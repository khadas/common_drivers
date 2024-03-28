#!/usr/bin/perl -W
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2024 Amlogic, Inc. All rights reserved.
#

my $arm64_dts = "arch/arm64/boot/dts/amlogic/";
my $arm_dts = "arch/arm/boot/dts/amlogic/";
my $err_cnt = 0;
my $exit = 0;

sub dts_check
{
	my @dts_path = @_;
	my $dts_diff = "";
	my $diff = `git diff  --name-only @dts_path | grep -E *dts\$`;
	if (!$diff)
	{
		return 0;
	}
	my $all_dts = `find @dts_path -name "*dts"`;
	my @diff_str = split /[\n]/, $diff;
	my @all_dts_str = split /[\n]/, $all_dts;
	my $diff_count = 0;
	my $all_dts_count = 0;
	my $count = 0;
	my $t = 0;
	my $board = "";
	my @all_arry = ("");
	my @diff_arry = ("");
	my @board_arry = ("");
	for (@diff_str)
	{
		$re = $_;
		if ($re=~m/^(.*?)\_/)
		{
			$dts_diff = $1;
			$dts_diff =~ /@dts_path/;
			$board=$';
			$dts_diff = $dts_diff."_";
		}
		if (!($_ =~ "pxp.dts"))
		{
			for (@diff_str)
			{
				if ($_ =~ $dts_diff)
				{
					$_ =~ /@dts_path/;
					$diff_arry->[$diff_count] = $';
					$diff_count = $diff_count+1;
				}
			}
			for (@all_dts_str)
			{
				if (!($_ =~ "pxp.dts"))
				{
					if ($_ =~ $dts_diff)
					{
						$_ =~ /@dts_path/;
						$all_arry->[$all_dts_count] = $';
						$all_dts_count=$all_dts_count+1;
					}
				}
			}
		}
		if ($diff_count != $all_dts_count)
		{
			$count=$count+1;
		}
		if(($t == 0) && ($count ==0))
		{
			$diff_count = 0;
			$all_dts_count = 0;
			next;
		}
		if($t != 0)
		{
			if ($board_arry->[$t-1] =~ $board)
			{
				$count=0;
				$diff_count = 0;
				$all_dts_count = 0;
			}
		}
		if ($count != 0)
		{
			$err_cnt += 1;
			$err_msg .= "	$err_cnt:You are modifying  the board $board DTS file in @dts_path \n";
			$err_msg .= "	  You have modified dts is ";
			while($diff_count > 0)
			{
				$err_msg .= "$diff_arry->[$diff_count-1] ";
				$diff_count=$diff_count-1;
			}
			$err_msg .= "\n";
			$err_msg .= "	  Please confirm should you modify other boards for $board in @dts_path too? ";

			$err_msg .= "\n";
			$count = 0;
			$diff_count = 0;
			$all_dts_count = 0;
			$board_arry->[$t]=$board;
			$t=$t+1;
		}
	}
}

sub check_dtsi
{
	my $arm64_file = `git diff  --name-only $arm64_dts| grep -E *dtsi\$`;
	my $arm_file = `git diff  --name-only $arm_dts| grep -E *dtsi\$`;
	my $all64_dtsi = `find $arm64_dts -name "*dtsi"`;
	my $all_dtsi = `find $arm_dts -name "*dtsi"`;
	my @all64_str = split /[\n]/, $all64_dtsi;
	my @all_str = split /[\n]/, $all_dtsi;
	my @diff64_str = split /[\n]/, $arm64_file;
	my @diff_str = split /[\n]/, $arm_file;
	my $count=0;
	my $ex=0;
	for(@diff_str)
	{
		$_ =~ (/arch\/arm\/boot\/dts\/amlogic\//);
		$dtsi_diff = $';
		for(@all64_str)
		{
			$_ =~ (/arch\/arm64\/boot\/dts\/amlogic\//);
			if ($' eq $dtsi_diff)
			{
				if(!$arm64_file)
				{
					$err_cnt += 1;
					$err_msg .= "	$err_cnt: maybe should modify $dtsi_diff in arm64\n";
					last;
				}
				for(@diff64_str)
				{
					$_ =~ (/arch\/arm64\/boot\/dts\/amlogic\//);
					if($' ne $dtsi_diff)
					{
						$count +=1;
					}
				}
				if($count != 0)
				{
					$err_cnt += 1;
					$err_msg .= "	$err_cnt: maybe should modify $dtsi_diff in arm64\n";
					$count = 0;
					$ex=1;
				}
			}
			if($ex==1)
			{
				$ex=0;
				last;
			}
		}
	}
	for(@diff64_str)
	{
		$_ =~ (/arch\/arm64\/boot\/dts\/amlogic\//);
		$dtsi_diff64 = $';
		for(@all_str)
		{
			$_ =~ (/arch\/arm\/boot\/dts\/amlogic\//);
			if ($' eq $dtsi_diff64)
			{
				if(!$arm_file)
				{
					$err_cnt += 1;
					$err_msg .= "	$err_cnt: maybe should modify $dtsi_diff64 in arm\n";
					last;
				}
				for(@diff_str)
				{
					$_ =~ (/arch\/arm\/boot\/dts\/amlogic\//);
					if($' ne $dtsi_diff64)
					{
						$count +=1;
					}
				}
				if($count != 0)
				{
					$err_cnt += 1;
					$err_msg .= "	$err_cnt: maybe should modify $dtsi_diff64 in arm\n";
					$count = 0;
					$ex=1;
				}
			}
			if($ex==1)
			{
				$ex=0;
				last;
			}
		}
	}
}

sub out_review
{
	my $out_msg = "";
	my $out_file = "../output/review.txt";

	if ($err_cnt)
	{
		$out_msg = <<END;
\$ git reset HEAD^ | ./scripts/amlogic/merge_check.pl
total $err_cnt errors.
${err_msg}

END

		$exit = 1;
		print $out_msg;
	}
	else
	{
		print "";
	}
}

dts_check($arm64_dts);
dts_check($arm_dts);
check_dtsi();
out_review();
exit $exit;
