@ECHO OFF

set SDL_VIDEO_WINDOW_POS=0,0

set BF_HOME=home
set BF_OPTIONS="-q%BF_HOME%" -r

IF EXIST engine\bloodfrontier_client.exe (
	engine\bloodfrontier_client.exe %BF_OPTIONS% %*
) ELSE (
	IF EXIST %BF_DIR%\engine\bloodfrontier_client.exe (
		pushd %BF_DIR%
		engine\bloodfrontier_client.exe %BF_OPTIONS% %*
		popd
	) ELSE (
		echo Unable to find 'bloodfrontier_client.exe'
	)
)
