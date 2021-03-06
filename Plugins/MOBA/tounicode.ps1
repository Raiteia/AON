
function tounicode($orginFile = $(throw "input file required"), $coding = 'Shift-JIS')
{
	$data = get-content $orginFile
	set-content -value $data $orginFile -encoding utf8
}

dir -r | % {if ($_ -like "*.cpp" -or
		$_ -like "*.c" -or
		$_ -like "*.cc" -or
		$_ -like "*.h" -or
		$_ -like "*.hpp") 
		{
			$_.fullname
			tounicode($_.fullname)
		}
	}