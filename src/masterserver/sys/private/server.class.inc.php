<?php
class server extends dbobject
{
	public static function main()
	{
		static $s;
		if ( !isset($s) )
		{
			$s = new server(null, 'server');
		}
		return $s;
	}
	public function findbyip($ip)
	{
		try
		{
			$sql = 'SELECT * FROM '.$this->__table.' WHERE ip = ? ORDER BY time DESC';
			$st = db_get()->prepare($sql);
			$st->bindValue(1, $ip);
			$st->execute();
			$r = $this->fetch($st);
			return $r;
		}
		catch(Exception $e)
		{
			throw $e;
			return false;
		}
	}
	function getactive()
	{
		try
		{
			$sql = 'SELECT * FROM `'.$this->__table.'` WHERE time > '.(time()-3600).' ORDER BY ip ASC;';
			$st = db_get()->prepare($sql);
			$st->execute();
			return $st;
		}
		catch(Exception $e)
		{
			throw $e;
			return false;
		}
	}
}

