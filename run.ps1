# Build the project
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force build
Set-Location build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
cmake --build .

# Run if build was successful
if ($LASTEXITCODE -eq 0) {
    Write-Host "`nCopying SDL2 DLL...`n"
    Copy-Item "C:/msys64/mingw64/bin/SDL2.dll" -Destination "." -ErrorAction SilentlyContinue
    
    Write-Host "`nLaunching fluid simulation...`n"
    
    # Start the process and wait for it to exit
    $env:PATH = "C:/msys64/mingw64/bin;$env:PATH"
    $process = Start-Process -FilePath "./fluid_sim.exe" -NoNewWindow -PassThru -Wait
    
    # Check if there was an error
    if ($process.ExitCode -ne 0) {
        Write-Host "`nSimulation exited with error code: $($process.ExitCode)"
        Write-Host "Press any key to continue..."
        $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    }
}
else {
    Write-Host "`nBuild failed. Press any key to continue..."
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
}

Set-Location ..