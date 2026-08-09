pushd %~dp0
pyinstaller --onefile --hidden-import=GameProtoParser GamePacketGenerator.py
MOVE /Y .\dist\GamePacketGenerator.exe .\GamePacketGenerator.exe
@RD /S /Q .\build
@RD /S /Q .\dist
DEL /S /F /Q .\GamePacketGenerator.spec
PAUSE
