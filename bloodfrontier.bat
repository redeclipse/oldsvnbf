@ECHO OFF

rem set SDL_VIDEO_WINDOW_POS=0,0
set BF_DIR=.
set BF_HOME=home
set BF_OPTIONS=-rbfa\init.cfg -q%BF_HOME%

IF EXIST bin\bloodfrontier_client.exe (
	bin\bloodfrontier_client.exe %BF_OPTIONS% %* 
) ELSE (
	IF EXIST %BF_DIR%\bin\bloodfrontier_client.exe (
		pushd %BF_DIR%
		bin\bloodfrontier_client.exe %BF_OPTIONS% %*
		popd
	) ELSE (
		echo Unable to find the Blood Frontier client
	)
)
