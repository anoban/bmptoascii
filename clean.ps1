Get-ChildItem *.exe -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.obj -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.o   -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.pdb -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.exp -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.dll -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.lib -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.ilk -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.i   -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
Get-ChildItem *.pch -Recurse | Foreach-Object {Remove-Item $_.FullName -Force}
