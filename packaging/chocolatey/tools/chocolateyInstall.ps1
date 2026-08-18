$ErrorActionPreference = 'Stop';
$packageName = 'unipaste'
$toolsDir = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$url64 = 'https://github.com/riccivr/unipaste/releases/download/v1.1.0/unipaste-v1.1.0-windows-x64.zip'
$checksum64 = '7eab52e4807323707876e543f7b071fa7fac0da85098606a5f8e4862ccbfd638'

$packageArgs = @{
  packageName   = $packageName
  unzipLocation = $toolsDir
  url64bit      = $url64
  checksum64    = $checksum64
  checksumType64= 'sha256'
}

Install-ChocolateyZipPackage @packageArgs
