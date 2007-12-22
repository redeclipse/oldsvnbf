<?php

class dbobject
{
	public $__data = array();
	public $__newdata = array();
	public $__cfetched = false;
	public $__table = NULL;
	function cnew()
	{
		//$c = __CLASS__;
		$c = get_class($this);
		$rd = new $c(NULL, $this->__table);
		return $rd;
	}
	function __construct($data=null, $table=null)
	{
		if ( !empty($table) )
		{
			$this->__table = $table;
		}
		if ( $data == null )
		{
			$this->__cfetched = false;
			$this->__data = array();
		}
		else
		{
			$this->__cfetched = true;
			$this->__data = $data;
		}
	}
	function fetch_byid($id, $usesql=NULL)
	{
		if ( !isset($this->__table) )
		{
			throw new Exception("No table set.");
			return false;
		}
		$this->__data = array();
		$this->__newdata = array();
		$this->__cfetched = false;
		try 
		{
			if ( isset($usesql) )
			{
				$sql = $usesql;
			}
			else
			{
				$sql = 'SELECT * FROM '.$this->__table.' WHERE id = ?';
			}
			$stmt = db_get()->prepare($sql);
			$stmt->execute(array($id));
			$r = $stmt->fetch(PDO::FETCH_ASSOC);
			if ( empty($r) )
			{
				return false;
			}
			//$c = __CLASS__;
		$c = get_class($this);	
			$rd = new $c($r, $this->__table);
			return $rd;
		}
		catch (Exception $e)
		{
			throw $e;
			return false;
		}
	}
	function __set($k, $v)
	{
		$this->__newdata[$k] = (string)$v;
	}
	function __get($k)
	{
		if ( isset($this->__newdata[$k]) )
		{
			return $this->__newdata[$k];
		}
		elseif ( isset($this->__data[$k]) )
		{
			return $this->__data[$k];
		}
		else
		{
			return null;
		}
	}
	function __isset($k)
	{
		return (isset($this->__data[$k])||isset($this->__newdata[$k]));
	}
	function save($useold=false)
	{
		$id = $this->__get('id');
		if ( isset($id) )
		{
			$r = $this->update($id);
			if ( $r )
			{
				return $id;
			}
			else
			{
				return false;
			}
		}
		else
		{
			$newid = $this->insert($useold);
			$this->__data['id'] = $newid;
			return $newid;
		}
	}
	function __unset($key)
	{
		unset($this->__newdata[$key]);
		unset($this->__data[$key]);
		return true;
	}
	function fetch($stmt)
	{
		if ( !is_object($stmt) )
		{
			return false;
		}
		$r = $stmt->fetch(PDO::FETCH_ASSOC);
		if ( empty($r) )
		{
			return false;
		}
		if ( empty($r) )
		{
			return false;
		}
		//$c = __CLASS__;
		//var_dump($c);
		//var_dump(get_class($this));
		$c = get_class($this);
		$rd = new $c($r, $this->__table);
		return $rd;
	}
	function update($id)
	{
		if ( !isset($this->__table) )
		{
			throw new Exception("No table set.");
			return false;
		}
		if ( count($this->__newdata) == 0 )
		{
			throw new Exception("No new data to update.");
			return false;
		}
		if (empty($id))
		{
			throw new Exception("No ID to update.");
			return false;
		}
		$sql  = 'UPDATE `'.$this->__table.'` SET ';
		$keys = array_keys($this->__newdata);
		$first = true;
		foreach($keys as $key)
		{
			if ( $first ) { $first = false; } else { $sql .= ', '; }
			$sql .= $key.' = :'.$key;
		}
		$sql .= ' WHERE `id` = '.(int)$id.'';
		//echo $sql;
		try
		{
			$stmt = db_get()->prepare($sql);
			foreach($this->__newdata as $key => $value)
			{
				$stmt->bindValue(':'.$key, $value);
			}
			if ( $stmt->execute() )
			{
				return $id;
			}
			else
			{
				return false;
			}
		}
		catch (Exception $e)
		{
			throw $e;
			return false;
		}
	}
	function insert($useold=false)
	{
		if ( !isset($this->__table) )
		{
			throw new Exception("No table set.");
			return false;
		}
 		if ( count($this->__newdata) == 0 && count($this->__data) == 0 )
		{
			throw new Exception("No data to insert");
			return false;
		}
		if ( $useold == true )
		{
			$keys = array_keys($this->__data);
		}
		else
		{
			$keys = array_keys($this->__newdata);
		}
		if ( $this->__isset('id') )
		{
			$this->__unset('id');
		}
		$sql = 'INSERT INTO '.$this->__table.' (';
		$sql .= implode(", ", $keys);
		$sql .= ') VALUES (';
		$subkeys = $keys;
		foreach($subkeys as $i => $key)
		{
			$subkeys[$i] = ':'.$key;
		}
		$sql .= implode(", ",$subkeys);
		$sql .= ');';
		try
		{
			$stmt = db_get()->prepare($sql);
			foreach($keys as $key)
			{
				$stmt->bindValue(':'.$key, $this->__get($key));
			}
			$stmt->execute();
		}
		catch (Exception $e)
		{
			throw $e;
			return false;
		}
		$r =  (int)(db_get()->lastInsertId());
		$this->__data['id'] = $r;
		foreach($this->__newdata as $key => $value)
		{
			$this->__data[$key] = $value;
		}
		$this->__newdata = array();
		return $r;
	}
	function delete($id = NULL)
	{
		if ( !isset($id) )
		{
			if ( !$this->__isset('id') )
			{
				throw new Exception("No ID to delete!");
				return false;
			}
			$id = $this->__get('id');
		}
		try
		{
			$sql = 'DELETE FROM '.$this->__table.' WHERE id = ? LIMIT 1';
			$stmt = db_get()->prepare($sql);
			$stmt->bindParam(1, $id);
			$stmt->execute();
			return true;
		}
		catch (Exception $e)
		{
			throw $e;
			return false;
		}
	}
	function change_check()
	{
		if ( $this->__cfetched === true )
		{
			$k = array_keys($this->__newdata);
			foreach($k as $key)
			{
				if (isset($this->__data[$key]))
				{
					if ( $this->__data[$key] != $this->__newdata[$key] )
					{
						return true;
					}
				}
			}
		}
		else
		{
			if ( count($this->__newdata) > 0 )
			{
				return true;
			}
		}
		return false;
	}
	function get($id=NULL)
	{
		if ( !isset($id) )
		{
			$sql = 'SELECT * FROM '.$this->__table.' ORDER BY id ASC';
			$stmt = db_get()->prepare($sql);
			$stmt->execute();
			return $stmt;
		}
		if ( is_int($id) || (is_string($id) && ctype_digit($id)) )
		{
			return $this->fetch_byid($id, 'SELECT * FROM '.$this->__table.' WHERE id = ? LIMIT 1');
		}
		else
		{
			return $this->fetch_byid($id, 'SELECT * FROM '.$this->__table.' WHERE name = ? LIMIT 1');
		}
	}
	function prepare($sql, $params = array())
	{
		try
		{
			$stmt = db_get()->prepare($sql);
			foreach($params as $key => $value)
			{
				$stmt->bindValue($key, $value);
			}
			$stmt->execute();
			return $stmt;
		}
		catch (Exception $e)
		{
			throw $e;
			return false;
		}
	}
	
	function reload()
	{
		$id = $this->__get('id');
		if ( empty($id) )
		{
			throw new Exception("Couldn't find an ID.");
			return false;
		}
		return $this->get($id);
	}
}
?>
