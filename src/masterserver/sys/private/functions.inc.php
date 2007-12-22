<?php
function cube_ping($ip, $time, $port)
{
	$max = 255;
	$timeout = 2;
	$addr = "udp://".$ip.":".$port;
	$sock = stream_socket_client($addr);
	if ( $sock === false )
	{
		return false;
	}
	stream_set_timeout($sock, $timeout);
	$s = pack('N', $time);
	if ( fputs($sock, $s) === false )
	{
		fclose($sock);
		return false;
	}
	$d = fgets($sock, $max);
	if ( $d == false )
	{
		fclose($sock);
		return false;
	}
	return $d;
}

