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

class xivo_application_service_asterisk_schedule extends xivo_application_asterisk_abstract
{
	var $_schedule		  = false;
	var $_schedule_path = false;
	var $_schedule_time = false;
	var $_dialaction	  = false;
	var $_contextmember	= false;
	var $_general      	= false;
	var $_sound	      	= array();
	var $_contextlist	  = array();

	public function __construct(&$service,&$dso,$param=null,$components=true)
	{
		parent::__construct($service);

		if(is_object($dso) === false)
			trigger_error('Invalid Datastorage object',E_USER_ERROR);

		$this->_dso = &$dso;

		if(($this->_schedule = &$this->_service->get_module('schedule')) === false)
			return(false);
		else if(($this->_schedule_path = &$this->_service->get_module('schedule_path')) === false)
			return(false);
		else if(($this->_schedule_time = &$this->_service->get_module('schedule_time')) === false)
			return(false);
		else if((bool) $components === true)
			$this->load_components();

		return(true);
	}

	function load_components()
	{
		$this->_components = true;

		if(($this->_dialaction = $this->get_abstract('dialaction',array('category' => 'schedule'))) === false
		|| ($this->_contextmember = $this->get_abstract('contextmember',array('type' => 'schedule'))) === false
		|| ($this->_general = &$this->_service->get_module('general')) === false)
			return(false);

		return(true);
	}

	function get($id,$disable=null)
	{
		$this->_info = $this->_origin = array();

		if(($this->_info['schedule'] = $this->_schedule->get($id,$disable)) === false)
			return(false);

		$this->_origin['schedule'] = $this->_schedule->get_origin();

		if($this->_components === false)
			return($this->_info);

		list($actiontype, $action) = $this->_explode_action($this->_info['schedule']['fallback_action']);

		$arg2 = $this->_info['schedule']['fallback_actionargs'];
		if(strstr($arg2, ';') !== false)
			$arg2 = array_fill_keys(explode(';', $arg2),1);

		$this->_info['dialaction'] = array('schedule_fallback' => array(
			'actiontype'  => $actiontype,
			'action'      => $action,
			// /!\ need both
			$actiontype => array(
				'action' => $action,
				'actionarg1' => $this->_info['schedule']['fallback_actionid'],
				'actionarg2' => $arg2
				),
			$action => array(
				'action' => $action,
				'actionarg1' => $this->_info['schedule']['fallback_actionid'],
				'actionarg2' => $arg2
				)
			)
		);

		$this->_get_opened($this->_info['schedule']['id']);
		$this->_get_closed($this->_info['schedule']['id']);

		return($this->_info);
	}

	function _explode_action($action)
	{
		$tokens = explode(':', $action, 2);
		$actiontype = $tokens[0];
		if (isset($tokens[1]))
		{
			$action = $tokens[1];
		}
		else
		{
			$action = $actiontype;
		}
		return array($actiontype, $action);
	}

	function _get_opened($scheduleid)
	{
		$this->_info['opened'] = $this->_schedule_time->get_all_where(array('schedule_id' => $scheduleid, 'mode' =>	'opened'));
	}

	function _get_closed($scheduleid)
	{
		$this->_info['closed'] = $this->_schedule_time->get_all_where(array('schedule_id' => $scheduleid, 'mode' =>	'closed'), null, false, 'id');

		for($i = 0; $i < count($this->_info['closed']); $i++)
		{
			$rs = $this->_info['closed'][$i];
			list($actiontype, $action) = $this->_explode_action($rs['action']);

			if(strstr($rs['actionargs'], ';') !== false)
				$rs['actionargs'] = array_fill_keys(explode(';', $rs['actionargs']),1);

			$this->_info['dialaction'][$i+1] = array(
				'actiontype' => $actiontype,
				'action'     => $action,
				// /!\ need both
				$actiontype => array(
					'action' => $action,
					'actionarg1' => $rs['actionid'],
					'actionarg2' => $rs['actionargs']
				),
				$action => array(
					'action' => $action,
					'actionarg1' => $rs['actionid'],
					'actionarg2' => $rs['actionargs']
				)
			);
		}
	}

	function get_elements()
	{
		$r = array();
		$r['schedule'] = $this->_schedule->get_element();
		$gene = $this->_general->get(1);
		$r['timezone'] = array('default' => $gene['timezone']);
		$r['dialaction'] = $this->_dialaction->get_element();

		return($r);
	}

	function get_schedules_search($search,$disable=null,$order=null,$limit=null,$assoc=false)
	{
		return($this->_get_schedules('search',$search,$disable,$order,$limit,$assoc));
	}

	function get_schedules_list($disable=null,$order=null,$limit=null,$assoc=false)
	{
		return($this->_get_schedules('list',null,$disable,$order,$limit,$assoc));
	}

	function _get_schedules($action,$search='',$disable=null,$order=null,$limit=null,$assoc=false)
	{
		$search = strval($search);

		switch($action)
		{
			case 'search':
				$rs = $this->_dso->search($search,$disable,$order,$limit);
				break;
			case 'list':
			default:
				$rs = $this->_schedule->get_all();
		}

		$this->_cnt = $this->_dso->get_cnt();

		if($rs === false || ($nb = count($rs)) === 0)
			return(false);

		$r = array();

		$_XOBJ = &dwho_gct::get('xivo_object');
		$modentity = &$_XOBJ->get_module('entity');

		for($i = 0;$i < $nb;$i++)
		{
			$r[$rs[$i]['id']] = $rs[$i];

			$ref = &$r[$rs[$i]['id']];
			$ref = $this->_schedule->_prepare($ref);

			if (($entity = $modentity->get($ref['entity_id'])))
				$ref['entity'] = $entity;
		}

		if(empty($r) === true)
			return(false);
		else if((bool) $assoc === false)
			return(array_values($r));

		return($r);
	}

	function get_dialaction_result()
	{
		return($this->_dialaction->get_result_for_display());
	}

	function get_dialaction_destination_list()
	{
		if(dwho_issa('schedule',$this->_info) === false)
			$id = 0;
		else
			$id = $this->_info['schedule']['id'];

		$r = $this->get_destination_list('schedule',$id);
		$r['sounds'] = $this->_dialaction->get_sound();

		return($r);
	}

	function get_sound()
	{
		if(empty($this->_sound) === false)
			return($this->_sound);
		else if(($sounds = &$this->_service->get_module('sounds')) === false)
			return(false);
		else if(($this->_sound = $sounds->get_list('playback','pathnoext')) !== false)
			asort($this->_sound);

		return($this->_sound);
	}

	function get_result_for_display()
	{
		$r = $this->get_result();

		if(is_array($r) === false)
			$r = array();

		if(dwho_issa('schedule',$r) === false)
			$r['schedule'] = array();

		$timebeg = $this->get_result_var('schedule','timebeg');
		$timeend = $this->get_result_var('schedule','timeend');

		$r['schedule']['timehourbeg'] = '';
		$r['schedule']['timeminbeg'] = '';
		$r['schedule']['timehourend'] = '';
		$r['schedule']['timeminend'] = '';

		if(dwho_has_len($timebeg) === true)
		{
			$timebeg = explode(':',$timebeg);
			$r['schedule']['timehourbeg'] = $timebeg[0];

			if(isset($timebeg[1]) === true)
				$r['schedule']['timeminbeg'] = $timebeg[1];
		}

		if(dwho_has_len($timeend) === true)
		{
			$timeend = explode(':',$timeend);
			$r['schedule']['timehourend'] = $timeend[0];

			if(isset($timeend[1]) === true)
				$r['schedule']['timeminend'] = $timeend[1];
		}

		return($r);
	}

	function set_add($arr)
	{
		return($this->_set('add',$arr));
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

		if(($action !== 'add' && $action !== 'edit') === true
		|| dwho_issa('schedule',$arr) === false)
			return(false);

		$this->set_schedule($arr['schedule'], $arr['dialaction']['schedule_fallback']);
		$this->set_opened($arr['opened']);
		$this->set_closed($arr['closed']);

		return(($this->get_errnb() < 1));
	}

	function set_schedule($arr, $dialaction)
	{
		$r = true;
		$arr['commented'] = false;

		$arr['fallback_action']     = $dialaction['actiontype'] .
			(array_key_exists('action',$dialaction)?':'.$dialaction['action']:'');
		$arr['fallback_actionid']   = $dialaction['actionarg1'];
		$arr['fallback_actionargs'] = is_array($dialaction['actionarg2']) ? implode(';',array_keys($dialaction['actionarg2'])).';':$dialaction['actionarg2'];


		if(($rs = $this->_schedule->chk_values($arr)) === false)
		{
			$r = false;
			$rs = $this->_schedule->get_filter_result();
			$this->_set_error('schedule',$this->_schedule->get_filter_error());
		}

		$this->_set_result('schedule',$rs);

		return($r);
	}

	function set_opened($arr)
	{
		$opened = array();
		foreach($arr as $rs)
		{
			$rs['mode'] = 'opened';
			$rs = $this->_schedule_time->chk_values($rs);
			$opened[] = $rs;
		}

		$this->_set_result('opened', $opened);
		return true;
	}

	function set_closed($arr)
	{
		foreach($arr as &$rs)
		{
			$rs['mode']   = 'closed';
			$rs['action'] = $rs['dialaction']['actiontype'] .
				(array_key_exists('action',$rs['dialaction'])?':'.$rs['dialaction']['action']:'');
			$rs['actionid']   = $rs['dialaction']['actionarg1'];
			$arg2 = $rs['dialaction']['actionarg2'];
			$rs['actionargs'] = is_array($arg2) ? implode(';',array_keys($arg2)).';':$arg2;

			unset($rs['dialaction']);

			$rs = $this->_schedule_time->chk_values($rs);
		}

		$this->_set_result('closed',$arr);
		return true;
	}

	function add()
	{
		$r = true;

		$this->_status = 'add';
		$this->_return = array();
		$scheduleid = null;

		if($this->get_errnb() > 0
		|| ($scheduleid = $this->add_schedule()) === false
		//|| $this->add_dialaction($scheduleid) === false
		//|| $this->add_contextmember($scheduleid) === false)
		|| $this->add_opened($scheduleid) === false
		|| $this->add_closed($scheduleid) === false)
		{
			$r = false;

			if($scheduleid !== null)
				$this->_reverse_add($scheduleid);
		}

		$this->_status = '';

		return($r);
	}

	function add_schedule()
	{
		if(($rs = $this->get_result('schedule')) === null
		|| $rs === false)
			return(false);

		return(($this->_return['schedule'] = $this->_schedule->add($rs)));
	}

	function add_opened($scheduleid)
	{
		foreach($this->get_result('opened') as $rs)
		{
			$rs['schedule_id'] = $scheduleid;
			$this->_schedule_time->add($rs);
		}

		return true;
	}

	function add_closed($scheduleid)
	{
		foreach($this->get_result('closed') as $rs)
		{
			$rs['schedule_id'] = $scheduleid;
			$this->_schedule_time->add($rs);
		}

		return true;
	}

	//TODO
	function _reverse_add($scheduleid)
	{
		if($this->get_errnb() > 0
		|| $this->_status !== 'add'
		|| ($scheduleid = dwho_ulongint($scheduleid)) === 0
		|| is_array($this->_return) === false
		|| empty($this->_return) === true)
			return(false);

		foreach($this->_return as $key => $val)
		{
			if($val === false)
				continue;

			switch($key)
			{
				case 'schedule':
					$this->_schedule->delete($scheduleid);
					break;
				case 'dialaction':
					$this->_dialaction->reverse_add();
					break;
				case 'contextmember':
					$this->_contextmember->reverse_add();
					break;
			}
		}

		return(true);
	}

	function edit()
	{
		$r = true;

		$this->_status = 'edit';
		$this->_return = array();

		if($this->get_errnb() > 0
		|| $this->edit_schedule() === false)
//		|| $this->edit_dialaction() === false
//		|| $this->edit_contextmember() === false)
		{
			$r = false;
			$this->_reverse_edit();
		}

		$this->_status = '';

		return($r);
	}

	function edit_schedule()
	{
		if(dwho_issa('schedule',$this->_info) === false
		|| ($rs = $this->get_result('schedule')) === null
		|| $rs === false)
			return(false);

		$rs['commented'] = $this->_info['schedule']['commented'];
		$this->_return['schedule'] = true;

		if($this->_schedule->edit($this->_info['schedule']['id'], $rs) == false
			|| $this->edit_opened($this->_info['schedule']['id']) == false
			|| $this->edit_closed($this->_info['schedule']['id']) == false)
		{
			$this->_return['schedule'] = false;
		}

		return($this->_return['schedule']);
	}

	function edit_opened($scheduleid)
	{
		$this->_schedule_time->delete_where(array('schedule_id' => $scheduleid, 'mode' => 'opened'));

		foreach($this->get_result('opened') as $rs)
		{
			$rs['schedule_id'] = $scheduleid;
			$this->_schedule_time->add($rs);
		}

		return true;
	}

	function edit_closed($scheduleid)
	{
		$this->_schedule_time->delete_where(array('schedule_id' => $scheduleid, 'mode' => 'closed'));

		foreach($this->get_result('closed') as $rs)
		{
			$rs['schedule_id'] = $scheduleid;
			$this->_schedule_time->add($rs);
		}

		return true;
	}

	// TODO
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
				case 'schedule':
					if(isset($this->_origin['schedule']) === false
					|| $this->_origin['schedule'] === false)
						$this->delete_schedule();
					else
						$this->_schedule->edit_origin($this->_origin['schedule']);
					break;
				case 'dialaction':
					$this->_dialaction->reverse_edit($this->_status);
					break;
				case 'contextmember':
					$this->_contextmember->reverse_edit($this->_status);
					break;
			}
		}

		return(true);
	}

	// TODO
	function delete()
	{
		$r = true;

		$this->_status = 'delete';
		$this->_return = array();

		if($this->delete_schedule() === false
		|| $this->delete_schedule_time() === false
		|| $this->delete_schedule_path() === false)
		{
			$r = false;
			$this->_reverse_delete();
		}

		$this->_status = '';
		return($r);
	}

	function delete_schedule()
	{
		if(dwho_issa('schedule',$this->_info) === false)
			return(false);

		$r = $this->_schedule->delete($this->_info['schedule']['id']);

		if($this->_status === 'delete')
			$this->_return['schedule'] = $r;

		return($r);
	}

	function delete_schedule_time()
	{
		$r = $this->_schedule_time->delete_where(array('schedule_id' => $this->_info['schedule']['id']));
		return $r;
	}

	function delete_schedule_path()
	{
		$r = $this->_schedule_path->delete_where(array('schedule_id' => $this->_info['schedule']['id']));

		return $r;
	}

	//TODO
	function _reverse_delete()
	{
		if($this->_status !== 'delete'
		|| is_array($this->_return) === false
		|| empty($this->_return) === true)
			return(false);

		foreach($this->_return as $key => $val)
		{
			if($val === false)
				continue;

			switch($key)
			{
				case 'schedule':
					if(isset($this->_origin['schedule']) === true
					&& $this->_origin['schedule'] !== false)
						$this->_schedule->add_origin($this->_origin['schedule']);
					break;
				case 'dialaction':
					$this->_dialaction->add_origin_list();
					break;
				case 'contextmember':
					$this->_contextmember->add_origin_list();
					break;
			}
		}

		return(true);
	}

	function enable()
	{
		$r = true;
		$this->_status = 'enable';
		$this->_return = array();

		if($this->enable_schedule() === false)
			$r = false;

		$this->_status = '';

		return($r);
	}

	function disable()
	{
		$r = true;
		$this->_status = 'disable';
		$this->_return = array();

		if($this->disable_schedule() === false)
			$r = false;

		$this->_status = '';

		return($r);
	}

	function enable_schedule()
	{
		return($this->_enable_disable_schedule(false));
	}

	function disable_schedule()
	{
		return($this->_enable_disable_schedule(true));
	}

	function _enable_disable_schedule($disable=false)
	{
		if(dwho_issa('schedule',$this->_info) === false)
			return(false);
		else if((bool) $disable === false)
			$r = $this->_schedule->enable($this->_info['schedule']['id']);
		else
			$r = $this->_schedule->disable($this->_info['schedule']['id']);

		if($this->_status === 'enable' || $this->_status === 'disable')
			$this->_return['schedule'] = $r;

		return($r);
	}

	function add_from_json()
	{
		if(($data = $this->_get_data_from_json()) === false
		|| $this->set_add($data) === false
		|| $this->add() === false)
			return(false);

		return(true);
	}
}
?>
