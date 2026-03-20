$acadPat = Get-Content "Application\AeSys\res\Hatches\acad.pat"
$patterns = @{}
$curName = $null; $curLines = @()
foreach ($rawLine in $acadPat) {
    $line = $rawLine.Trim()
    if ($line -match '^\*([^,]+),(.*)') {
        if ($null -ne $curName) { $patterns[$curName] = $curLines }
        $curName = $Matches[1].Trim(); $curLines = @()
    } elseif ($line -ne '' -and -not $line.StartsWith(';;') -and $null -ne $curName) {
        $curLines += $line
    }
}
if ($null -ne $curName) { $patterns[$curName] = $curLines }

$pi = [Math]::PI

function Convert-PatternLine {
    param([string]$acadLine)
    $fields = $acadLine -split ',\s*'
    $angleDeg = [double]$fields[0]
    $angleRad = $angleDeg * $pi / 180.0
    $baseX = $fields[1].Trim()
    $baseY = $fields[2].Trim()
    $shift = $fields[3].Trim()
    $spacing = $fields[4].Trim()
    $parts = [System.Collections.Generic.List[string]]::new()
    $fmtAngle = "{0:G7}" -f $angleRad
    if ($fmtAngle -eq "0") { $fmtAngle = "0" }
    $parts.Add($fmtAngle)
    $parts.Add($baseX)
    $parts.Add($baseY)
    $parts.Add($shift)
    $parts.Add($spacing)
    if ($fields.Count -le 5) {
        $parts.Add("1.E16")
    } else {
        for ($i = 5; $i -lt $fields.Count; $i++) {
            $parts.Add($fields[$i].Trim())
        }
    }
    return ($parts -join ", ")
}

$indexOrder = @(
    @(1, "PEG1"), @(2, "PEG2"),
    @(3, "ANGLE"), @(4, "ANSI31"), @(5, "ANSI32"), @(6, "ANSI33"), @(7, "ANSI34"),
    @(8, "ANSI35"), @(9, "ANSI36"), @(10, "ANSI37"), @(11, "ANSI38"),
    @(12, "BOX"), @(13, "BRICK"), @(14, "CLAY"), @(15, "CORK"), @(16, "CROSS"),
    @(17, "DASH"), @(18, "DOLMIT"), @(19, "DOTS"), @(20, "EARTH"), @(21, "ESCHER"),
    @(22, "FLEX"), @(23, "GRASS"), @(24, "GRATE"), @(25, "HEX"), @(26, "HONEY"),
    @(27, "HOUND"), @(28, "INSUL"), @(29, "MUDST"), @(30, "NET3"), @(31, "PLAST"),
    @(32, "PLASTI"), @(33, "SACNCR"), @(34, "SQUARE"), @(35, "STARS"), @(36, "SWAMP"),
    @(37, "TRANS"), @(38, "TRIANG"), @(39, "ZIGZAG"),
    @(40, "AR-B816"), @(41, "AR-B816C"), @(42, "AR-B88"), @(43, "AR-BRELM"),
    @(44, "AR-BRSTD"), @(45, "AR-CONC"), @(46, "AR-HBONE"), @(47, "AR-PARQ1"),
    @(48, "AR-RROOF"), @(49, "AR-RSHKE"), @(50, "AR-SAND")
)

$output = [System.Collections.Generic.List[string]]::new()
$missingPatterns = @()

foreach ($entry in $indexOrder) {
    $idx = $entry[0]
    $name = $entry[1]
    $output.Add("! $($name.ToLower())")
    if ($name -eq "PEG1") {
        $output.Add("0, 0, 0, 0, 1, 1.E16")
    } elseif ($name -eq "PEG2") {
        $output.Add("0, 0, 0, 0, 1, 1.E16")
        $output.Add("1.570796, 0, 0, 0, 1, 1.E16")
    } else {
        if ($null -eq $patterns[$name]) {
            $missingPatterns += $name
            $output.Add("0, 0, 0, 0, 1, 1.E16")
        } else {
            foreach ($acadLine in $patterns[$name]) {
                $converted = Convert-PatternLine $acadLine
                $output.Add($converted)
            }
        }
    }
}
$output.Add("! END")

if ($missingPatterns.Count -gt 0) {
    Write-Output "WARNING: Missing patterns: $($missingPatterns -join ', ')"
}

$outputPath = "Application\AeSys\res\Hatches\DefaultSet.txt"
$output | Set-Content -Path $outputPath -Encoding UTF8
Write-Output "Generated $outputPath with $($indexOrder.Count) patterns, $($output.Count) lines"
