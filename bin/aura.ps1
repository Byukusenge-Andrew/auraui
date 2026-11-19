$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$CliPath = Join-Path $ScriptDir "..\auraui\cli\auraui.py"
python $CliPath @args
