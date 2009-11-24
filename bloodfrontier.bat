@ECHO OFF

rem set SDL_VIDEO_WINDOW_POS=0,0
set BF_DIR=.
set BF_OPTIONS=-r

IF EXIST bin\bfclient.exe (
	start bin\bfclient.exe %BF_OPTIONS% %* 
) ELSE (
	IF EXIST %BF_DIR%\bin\bfclient.exe (
		pushd %BF_DIR%
		start bin\bfclient.exe %BF_OPTIONS% %*
		popd
	) ELSE (
		echo Unable to find the Blood Frontier client
		pause
	)
)
