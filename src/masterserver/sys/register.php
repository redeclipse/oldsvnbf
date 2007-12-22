<?php

require_once("private/init.inc.php");

if ( !isset($_SERVER['REMOTE_ADDR']) )
{
	die("No IP");
}
if ( !isset($_SERVER['HTTP_USER_AGENT']) )
{
	die("No user agent");
}
if ( preg_match("/bloodfrontierserver/i", $_SERVER['HTTP_USER_AGENT']) == 0 )
{
	die("Incorrect user agent.");
}
$ip = $_SERVER['REMOTE_ADDR'];

$s = server::main();

if ( cube_ping($ip, time(), 28796) === false )
{
	if ( cube_ping($ip, time(), 28786) !== false )
	{
		die("Your server version is outdated. Please use the patch from http://sf.net/projects/bloodfrontier/\n");
	}
	else
	{
		die("Your server did not respond to the ping.");
	}
}

$server = $s->findbyip($ip);
if ( $server === false )
{
	$server = $s->cnew();
	$server->time = time();
	$server->ip = $ip;
	$id = $server->save();
	if ( $id )
	{
		echo "Your server has been registered.";
		exit();
	}
	else
	{
		die("Your server could not be registered.");
	}
}
$server->time = time();

$id = $server->save();
if ( $id )
{
	echo "Your server has been updated.";
}
else
{
	echo "Your server has failed to update";
}
