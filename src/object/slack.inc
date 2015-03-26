<?php
#
# XiVO Web-Interface
# Copyright (C) 2013-2014  Avencall
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

class xivo_slack
{

	private $_ressource = array();

	function &get_ressource($name,$param=null)
	{
		$r = false;

		$name = (string) $name;

		if($this->load_ressource($name,$param) === true)
			$r = &$this->_ressource[$name];

		return(($ref = &$r));
	}

	public function load_ressource($name,$param=null)
	{
		if(isset($this->_ressource[$name]) === true)
			return(true);
		else if(($class = $this->load_ressource_class($name)) === false)
		{
			trigger_error('Module '.$name.' does not exist in XIVO Object',E_USER_ERROR);
			return(false);
		}
		else
			$this->_ressource[$name] = new $class($this,$param);

		return(true);
	}

	public function load_ressource_class($name)
	{
		$this->_load_ressource_abstract();

		$name = (string) $name;

		$file = $name.'.inc';

		$dir = dwho_file::joinpath(XIVO_PATH_OBJECT, 'slack', $name);

		$class = 'slack_ressource_'.$name;

		if(dwho_file::load_file($file,$dir,true) !== false && class_exists($class) === true)
			return($class);

		return(false);
	}

	function _load_ressource_abstract()
	{
		$dir = dwho_file::joinpath(XIVO_PATH_OBJECT, 'slack', 'abstract');

		$class = 'xivo_slack_ressource_abstract';

		if(dwho_file::load_file('abstract.inc',$dir,true) !== false && class_exists($class) === true)
			return($class);

		return(false);
	}
}

?>