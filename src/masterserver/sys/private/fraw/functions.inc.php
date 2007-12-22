<?php
function db_get()
{
	static $db;
	if ( !isset($db) )
	{
//		$db = new pdo('mysql:host=localhost;dbname=msl', 'msl', 'msl');
//var_dump(file_exists("private/write/main.db"));
		$db = new pdo('sqlite:private/write/main.db');
		$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	}
	return $db;
}
