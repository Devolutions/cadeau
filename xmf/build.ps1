
foreach ($Architecture in @('x64', 'arm64')) {
	$BuildDir = "build-${Architecture}"
	$DotNetRid = "win-${Architecture}"
	$MsvcArch = @{'x64'='x64';'arm64'='ARM64'}[$Architecture]
	Remove-Item -Path $BuildDir -Recurse -Force
	New-Item -Path $BuildDir -ItemType Directory -ErrorAction SilentlyContinue | Out-Null
	cmake -G "Visual Studio 17 2022" -A $MsvcArch . -B $BuildDir
	cmake --build $BuildDir --config Release
	New-Item -Path ".\runtimes\${DotNetRid}\native" -ItemType Directory -ErrorAction SilentlyContinue | Out-Null
	Copy-Item "$BuildDir\libxmf\Release\libxmf.dll" ".\runtimes\${DotNetRid}\native\xmf.dll" -Force
}

New-Item -Path ".\package" -ItemType Directory -ErrorAction SilentlyContinue | Out-Null
& dotnet pack .\dotnet\Devolutions.Xmf -c Release -o .\package
