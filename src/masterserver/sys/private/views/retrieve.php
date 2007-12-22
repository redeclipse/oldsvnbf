<?php

header("Content-type: text/plain");

$s = server::main();

while ( $server = $s->fetch($servers) )
{
	echo "addserver ".$server->ip."\n";
}

