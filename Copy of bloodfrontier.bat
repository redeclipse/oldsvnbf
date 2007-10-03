@ECHO OFF

set BF_DIR=.
set BF_HOME=home
set BF_OPTIONS=-q%BF_HOME% -r -k%BF_DIR%

IF EXIST engine\bloodfrontier_client.exe (
	engine\bloodfrontier_client.exe %BF_OPTIONS% %*
) ELSE (
	IF EXIST %BF_DIR%\engine\bloodfrontier_client.exe (
		pushd %BF_DIR%
		engine\bloodfrontier_client.exe %BF_OPTIONS% %*
		popd
	) ELSE (
		echo Unable to find the Blood Frontier client
	)
)
