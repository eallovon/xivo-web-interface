<?php

#
# XiVO Web-Interface
# Copyright (C) 2006-2014  Avencall
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

dwho_file::required(array(XIVO_PATH_OBJECT,'application','service','ipbx','asterisk.inc'),true);

class xivo_application_service_asterisk_ctiphonehints extends xivo_application_asterisk_abstract
{
	var $_ctiphonehints = false;

	public function __construct(&$service,$param=null)
	{
		parent::__construct($service);

		if(($this->_ctiphonehints = &$this->_service->get_module('ctiphonehints')) === false)
			return(false);

		return(true);
	}

	function get_phonehints_list($order=null,$limit=null,$assoc=false,$idgroup=1)
	{
		$w = array();
		$w['idgroup'] = (int) $idgroup;

		if(($rs = $this->_ctiphonehints->get_all_where($w,null,true,$order,$limit)) === false
		|| ($nb = count($rs)) === 0)
		{
			$this->_cnt = $this->_ctiphonehints->get_cnt();
			return(false);
		}

		$r = $info = array();
		for($i = 0;$i < $nb;$i++)
		{
			$info['ctiphonehints'] = $rs[$i];
			$r[$info['ctiphonehints']['id']] = $info;
		}

		if(empty($r) === true)
			return(false);
		else if((bool) $assoc === false)
			return(array_values($r));

		return($r);
	}

	function get($id)
	{
		$this->_info = $this->_origin = array();

		$phonehints = $this->_ctiphonehints->get($id);
		if($phonehints === false)
			return(false);

		$this->_info['phonehints'] = $phonehints;

		return($this->_info);
	}

	function get_nb($arr=null,$disable=null)
	{
		return($this->_ctiphonehints->get_nb($arr,$disable));
	}

	function get_elements()
	{
		$r = array();
		$r['phonehints'] = $this->_ctiphonehints->get_element();

		return($r);
	}

	function set_edit($arr)
	{
		if(empty($this->_info) === true)
			return(false);

		return($this->_set('edit',$arr));
	}

	function _set($action,$arr)
	{
		$this->_reset();

		if(($action !== 'edit') === true
		|| dwho_issa('phonehints',$arr) === false)
			return(false);

		$this->set_phonehints($arr['phonehints']);

		return(($this->get_errnb() < 1));
	}

	function set_phonehints($arr)
	{
		if(is_array($arr) === false)
		{
			$this->_set_result('phonehints',null);
			$this->_set_error('phonehints','invalid data');
			return(false);
		}
		else if(($rs = $this->_ctiphonehints->chk_values($arr)) === false)
		{
			$this->_set_result('phonehints',$this->_ctiphonehints->get_filter_result());
			$this->_set_error('phonehints',$this->_ctiphonehints->get_filter_error());
			return(false);
		}

		$this->_set_result('phonehints',$rs);

		return(true);
	}

	function edit()
	{
		$r = true;

		$this->_status = 'edit';
		$this->_return = array();

		if($this->get_errnb() > 0
		|| $this->edit_phonehints() === false)
		{
			$r = false;
			$this->_reverse_edit();
		}

		$this->_status = '';

		return($r);
	}

    function edit_phonehints()
    {
        if(dwho_issa('phonehints',$this->_info) === false
        || ($rs = $this->get_result('phonehints')) === null
        || $rs === false)
            return(false);

        return(($this->_return['phonehints'] = $this->_ctiphonehints->edit($this->_info['phonehints']['id'],$rs)));
    }

	function _reverse_edit()
	{
		if($this->get_errnb() > 0
		|| $this->_status !== 'edit'
		|| is_array($this->_return) === false
		|| empty($this->_return) === true)
			return(false);

		foreach($this->_return as $key => $val)
		{
			if($val === false)
				continue;

			switch($key)
			{
				case 'phonehints':
					if(isset($this->_origin['phonehints']) === false
					|| $this->_origin['phonehints'] === false)
						$this->_delete_phonehints();
					else
						$this->_ctiphonehints->edit_origin($this->_origin['phonehints']);
					break;
			}
		}

		return(true);
	}

	function _delete_phonehints()
	{
		if(dwho_issa('phonehints',$this->_info) === false)
			return(false);

		$r = $this->_ctiphonehints->delete($this->_info['phonehints']['id']);

		if($this->_status === 'delete')
			$this->_return['phonehints'] = $r;

		return($r);
	}
}

?>
