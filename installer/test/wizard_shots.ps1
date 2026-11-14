# Walk the (TESTMODE) installer wizard and save a PNG of every page.
#   powershell -NoProfile -Command "& ([scriptblock]::Create((Get-Content -Raw '<this file>'))) <setup.exe> <testroot> <iso> <outdir> [prefix]"
# (run via a scriptblock: -File on a \\wsl.localhost path can stop at a script-security prompt)
param([string]$Setup, [string]$Root, [string]$Iso, [string]$Out, [string]$Prefix = "page")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public static class W {
  [DllImport("user32.dll")] public static extern IntPtr FindWindowEx(IntPtr p, IntPtr a, string c, string t);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetDlgCtrlID(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr h, int m, IntPtr w, string l);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, int m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, int m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr p, EnumProc f, IntPtr l);
  public static IntPtr[] Children(IntPtr p) { var l = new System.Collections.Generic.List<IntPtr>(); EnumChildWindows(p, (h, x) => { l.Add(h); return true; }, IntPtr.Zero); return l.ToArray(); }
  public static string Text(IntPtr h) { var s = new StringBuilder(2048); GetWindowText(h, s, 2048); return s.ToString(); }
}
"@
[W]::SetProcessDPIAware() | Out-Null
$n = 0
function Shot($h, $name) {
  Start-Sleep -Milliseconds 600
  $r = New-Object W+RECT; [W]::GetWindowRect($h, [ref]$r) | Out-Null
  $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
  $g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
  [W]::PrintWindow($h, $dc, 2) | Out-Null; $g.ReleaseHdc($dc); $g.Dispose()
  $script:n++; $f = Join-Path $Out ("{0}_{1:D2}_{2}.png" -f $Prefix, $script:n, $name)
  $bmp.Save($f, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose(); Write-Output "saved $f"
  # text "screenshot": every visible control's class, state and text (PrintWindow
  # does not always render static text, so this is the reliable record)
  $lines = foreach ($k in [W]::Children($h)) { if ([W]::IsWindowVisible($k)) {
      $c = New-Object System.Text.StringBuilder 64; [W]::GetClassName($k, $c, 64) | Out-Null
      $en = if ([W]::IsWindowEnabled($k)) { "" } else { " (disabled)" }
      $extra = ""
      if ($c.ToString() -eq "msctls_progress32") { $extra = " pos=" + [W]::SendMessage($k, 0x0408, [IntPtr]::Zero, [IntPtr]::Zero) }
      if ($c.ToString() -eq "Button") { $st = [W]::SendMessage($k, 0x00F0, [IntPtr]::Zero, [IntPtr]::Zero); if ($st -ne 0) { $extra = " checked" } }
      "[{0}]{1}{2} {3}" -f $c, $en, $extra, ([W]::Text($k) -replace "`r`n", " / ") } }
  Set-Content -Path ($f -replace '\.png$', '.txt') -Value $lines -Encoding UTF8
}
function Click($dlg, $btn) { [W]::PostMessage($dlg, 0x0111, [IntPtr][W]::GetDlgCtrlID($btn), $btn) | Out-Null }
function Next($h) { Click $h ([W]::GetDlgItem($h, 1)); Start-Sleep -Milliseconds 1200 }
function WaitNext($h, $sec) { $t = 0; while (-not [W]::IsWindowEnabled([W]::GetDlgItem($h, 1)) -and $t -lt $sec * 10) { Start-Sleep -Milliseconds 100; $t++ } }
function Inner($h) { [W]::FindWindowEx($h, [IntPtr]::Zero, "#32770", $null) }
function Header($h) { [W]::Text([W]::GetDlgItem($h, 1037)) }

# CreateProcess (not ShellExecute): no "Open File - Security Warning" for the UNC path
$psi = New-Object System.Diagnostics.ProcessStartInfo $Setup, "/TESTROOT=$Root /D=$Root\PF\SMB2"
$psi.UseShellExecute = $false
$p = [System.Diagnostics.Process]::Start($psi)
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 100 -and $h -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 200; $p.Refresh(); $h = $p.MainWindowHandle }
Start-Sleep -Milliseconds 800
Shot $h "welcome"; Next $h
Shot $h "license"; Next $h
if ((Header $h) -match "Location") { Shot $h "directory"; Next $h }
Shot $h "components"; Next $h
$in = Inner $h; $edit = [W]::FindWindowEx($in, [IntPtr]::Zero, "Edit", $null)
Shot $h "iso_empty"
[W]::SendMessage($edit, 0x000C, [IntPtr]::Zero, $Iso) | Out-Null; Start-Sleep -Milliseconds 400
Shot $h "iso_selected"; Next $h
# InstFiles advances by itself to the extraction page
for ($t = 0; $t -lt 600 -and -not ((Header $h) -match "Extracting"); $t++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 2500; Shot $h "extracting"
# wait until extraction ends: Next enabled (done) or the error buttons shown
$in = Inner $h
$pick = [W]::FindWindowEx($in, [IntPtr]::Zero, "Button", "&Choose another disc image...")
for ($t = 0; $t -lt 3000; $t++) {
  if ([W]::IsWindowEnabled([W]::GetDlgItem($h, 1)) -or ($pick -ne [IntPtr]::Zero -and [W]::IsWindowVisible($pick))) { break }
  if ($t -eq 150) { Shot $h "extracting_later" }
  Start-Sleep -Milliseconds 100
}
Start-Sleep -Milliseconds 500
if ($pick -ne [IntPtr]::Zero -and [W]::IsWindowVisible($pick)) {
  Shot $h "extract_error"
  $skip = [W]::FindWindowEx($in, [IntPtr]::Zero, "Button", "&Skip - choose it later in the game")
  Click ([W]::GetParent($skip)) $skip; Start-Sleep -Milliseconds 600
  Shot $h "extract_skipped"
} else { Shot $h "extract_done" }
Next $h
Start-Sleep -Milliseconds 500
# untick "Launch now" (id 1203) before Finish
$in = Inner $h; $run = [W]::GetDlgItem($in, 1203)
Shot $h "finish"
[W]::SendMessage($run, 0x00F1, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
Next $h
$p.WaitForExit(15000) | Out-Null
Write-Output "setup exit $($p.ExitCode)"
