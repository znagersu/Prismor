Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public class WindowCaptureHelper {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    public static List<IntPtr> FindWindows(uint pid) {
        var list = new List<IntPtr>();
        EnumWindows((hWnd, lp) => {
            uint p;
            GetWindowThreadProcessId(hWnd, out p);
            if (p == pid && IsWindowVisible(hWnd)) {
                list.Add(hWnd);
            }
            return true;
        }, IntPtr.Zero);
        return list;
    }
}
"@

$proc = Start-Process -FilePath "C:\Users\endio\.gemini\antigravity\scratch\Prismor\scratch\Prismor_1.0.15.4_user_simulation.exe" -PassThru
Write-Output "Launched process PID $($proc.Id). Waiting 4 seconds for GitHub release query..."
Start-Sleep -Milliseconds 4500

$hwnds = [WindowCaptureHelper]::FindWindows($proc.Id)
Write-Output "Found $($hwnds.Count) window(s)."

foreach ($h in $hwnds) {
    $r = New-Object WindowCaptureHelper+RECT
    [WindowCaptureHelper]::GetWindowRect($h, [ref]$r)
    $w = $r.Right - $r.Left
    $ht = $r.Bottom - $r.Top
    Write-Output "Evaluating window: ${w}x${ht}"
    if ($w -gt 500) {
        $bmp = New-Object System.Drawing.Bitmap($w, $ht)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size($w, $ht)))
        $outPath = "C:\Users\endio\.gemini\antigravity\brain\f2afe8f2-281f-439b-882d-e0a5497d8e9e\prismor_user_live_network_prompt.png"
        $bmp.Save($outPath)
        $g.Dispose()
        $bmp.Dispose()
        Write-Output "Saved live capture to $outPath"
        break
    }
}

Stop-Process -Id $proc.Id -Force
Write-Output "Process stopped."
