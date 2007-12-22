<?php

require_once("private/init.inc.php");

$s = server::main();
$servers = $s->getactive();

require_once("private/views/retrieve.php");

