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


dwho::load_class('dwho_network');
dwho::load_class('dwho_http');

dwho_file::required(array(XIVO_PATH_OBJECT,'application','service','ipbx','asterisk.inc'),true);

class xivo_application_service_asterisk_user extends xivo_application_asterisk_abstract
{
	protected $_dso				= null;
	protected $_userfeatures 	= false;
	protected $_rightcall		= false;
	protected $_dialaction		= false;
	protected $_queuemember		= null;
	protected $_voicemail		= null;
	protected $_agentfeatures	= null;
	protected $_linefeatures	= false;
	protected $_line			= false;
	protected $_cti_profile     = false;
	protected $_appentity		= false;
	protected $_entity			= false;
	protected $_user_line		= false;
	protected $_phonefunckey	= null;
	protected $_provdconfig		= false;
	protected $_provddevice		= false;
	protected $_device		 	= false;
	protected $_sccpdevice		= false;
	protected $_musiconhold		= array();
	protected $_profileclientlist	= array();
	protected $_conf			= array();
	protected $_param			= array();
	protected $_destination		= null;
	protected $_schedule_path 	= false;
	protected $_func_key_manager	= false;
	protected $_callfilter = false;
	protected $_callfiltermember = false;
	protected $_valid_services	= array('vmuserpurge',
										'vmusermsg',
										'enablevm',
										'phonestatus',
										'recsnd',
										'calllistening',
										'directoryaccess',
										'fwdundoall',
										'pickup',
										'callrecord',
										'incallfilter',
										'enablednd',
										'automon'
	);
	protected $_forward_services	= array('fwdrna',
											'fwdbusy',
											'fwdunc');
	protected $_func_key_agent = null;

	public function __construct(&$service,&$dso,$param=null,$components=true)
	{
		parent::__construct($service);

		if(is_object($dso) === false)
			trigger_error('Invalid Datastorage object',E_USER_ERROR);

		$this->_dso = &$dso;

		if(($this->_userfeatures = &$this->_service->get_module('userfeatures')) === false
		|| $this->load_init($param) === false)
			return(false);
		else if((bool) $components === true)
			$this->load_components();

		$this->_load_config(dirname(__FILE__));

		if(($size = $this->_verify_can_upload_file()) === false)
			die();

		$this->_conf['import']['file']['size'] = $size;

		// init random generator
		mt_srand(time()+mt_rand());

		return(true);
	}

	function load_init($param=array())
	{
		$this->_reset_info();
		$this->_reset_params();
		$this->_reset();

		return(true);
	}

	function load_components()
	{
		$this->_components = true;

		if(($this->_sccpdevice = &$this->_service->get_module('sccpdevice')) === false
		|| ($this->_device = &$this->_service->get_application('device')) === false
		|| ($this->_extensions = &$this->_service->get_module('extensions')) === false
		|| ($this->_rightcall = $this->get_abstract('rightcall',array('type' => 'user'))) === false
		|| ($this->_dialaction = $this->get_abstract('dialaction',array('category' => 'user'))) === false
		|| ($this->_line = $this->_service->get_application('line')) === false
		|| ($this->_schedule_path = &$this->_service->get_module('schedule_path')) === false
		|| ($this->_linefeatures = $this->_service->get_module('linefeatures')) === false
		|| ($this->_user_line = $this->_service->get_module('user_line')) === false
		|| ($_XOBJ = &dwho_gct::get('xivo_object')) === false
		|| ($this->_appentity = &$_XOBJ->get_application('entity')) === false
		|| ($this->_entity = &$_XOBJ->get_module('entity')) === false
		|| ($this->_provdconfig = &$_XOBJ->get_module('provdconfig')) === false
		|| ($_RAPI = &dwho_gct::get('xivo_confd')) === false
		|| ($this->_cti_profile = $_RAPI->get_ressource('cti_profile')) === false
		|| ($this->_provddevice = &$_XOBJ->get_module('provddevice')) === false
		|| ($_MGR = &dwho_gct::get('xivo_manager')) === false
		|| ($this->_func_key_manager = &$_MGR->get_ressource('func_key')) === false
		|| ($this->_func_key_agent = $this->_service->get_module('func_key_dest_agent')) === false
		|| ($this->_callfilter = $this->_service->get_application('callfilter')) === false
		|| ($this->_callfiltermember = $this->_service->get_module('callfiltermember')) === false)
			return(false);

		$this->_queuemember = &$this->_service->get_module('queuemember');
		$this->_voicemail = &$this->_service->get_application('voicemail');
		$this->_agentfeatures = &$this->_service->get_module('agentfeatures');
		$this->_phonefunckey = &$this->_service->get_module('phonefunckey');

		return(true);
	}

	function get($id,$disable=null,$nocomponents=null)
	{
		$this->_info = $this->_origin = array();

		$this->_info['userfeatures'] = $this->_userfeatures->get($id);

		if ($this->_info['userfeatures'] === false)
			return(false);

		$this->_origin['userfeatures'] = $this->_userfeatures->get_origin();

		if($this->_components === false)
			return($this->_info);

		if(dwho_ak('entity',$nocomponents,true) === false)
			$this->_get_entity();

		if(dwho_ak('user_line',$nocomponents,true) === false)
			$this->_get_user_line();

		if(dwho_ak('linefeatures',$nocomponents,true) === false)
			$this->_get_linefeatures();

		if(dwho_ak('exten',$nocomponents,true) === false)
			$this->_get_exten();

		if(dwho_ak('groupmember',$nocomponents,true) === false)
			$this->_get_groupmember();

		if(dwho_ak('queuemember',$nocomponents,true) === false)
			$this->_get_queuemember();

		if(dwho_ak('rightcall',$nocomponents,true) === false)
			$this->_get_rightcall($this->_info['userfeatures']['id']);

		if(dwho_ak('dialaction',$nocomponents,true) === false)
			$this->_get_dialaction($this->_info['userfeatures']['id']);

		if(dwho_ak('voicemail',$nocomponents,true) === false)
			$this->_get_voicemail();

		if(dwho_ak('phonefunckey',$nocomponents,true) === false)
			$this->_get_phonefunckey();

		// schedule
		$this->_info['schedule_id'] = 0;
		$where = array('path' => 'user', 'pathid' => $this->_info['userfeatures']['id']);
		if(($schedule = $this->_schedule_path->get_where($where)) !== false)
			$this->_info['schedule_id'] = intval($schedule['schedule_id']);

		return($this->_info);
	}

	function _get_user_line()
	{
		$this->_info['user_line'] = false;
		$this->_origin['user_line'] = false;

		if(is_object($this->_user_line) === false)
			return(false);

		$user_id = (int) $this->_info['userfeatures']['id'];

		$this->_info['user_line'] = $this->_user_line->find_by_user_id($user_id);
		$this->_origin['user_line'] = $this->_user_line->get_origin_list();

		return($this->_info['user_line']);
	}

	function _get_linefeatures()
	{
		$this->_info['linefeatures'] = false;
		$this->_origin['linefeatures'] = false;

		if(is_object($this->_linefeatures) === false
		|| $this->_info['user_line'] === false
		|| count($this->_info['user_line']) === 0)
			return(false);

		$this->_info['linefeatures'] = array($this->_linefeatures->get($this->_info['user_line'][0]['line_id']));
		$this->_origin['linefeatures'] = $this->_linefeatures->get_origin_list();

		return($this->_info['linefeatures']);
	}

	function _get_exten()
	{
		$this->_info['exten'] = false;
		$this->_origin['exten'] = false;

		if(isset($this->_info['linefeatures'][0]) === true
		&& ($this->_info['exten'] = $this->_extensions->get_exten($this->_info['linefeatures'][0]['number'],
									      $this->_info['linefeatures'][0]['context'])) !== false)
			$this->_origin['exten'] = $this->_extensions->get_origin();

		return($this->_info['exten']);
	}

	function _get_entity()
	{
		$this->_info['entity'] = false;
		$this->_origin['entity'] = false;

		if(is_object($this->_appentity) === false)
			return(false);

		$this->_info['entity']  = $this->_appentity->get($this->_info['userfeatures']['entityid'],null,'internal');

		return($this->_info['entity']);
	}

	function _get_groupmember()
	{
		return($this->_get_member_type('group'));
	}

	function _get_queuemember()
	{
		return($this->_get_member_type('queue'));
	}

	function _get_member_type($type)
	{
		if($type !== 'group' && $type !== 'queue')
			return(false);

		$membertype = $type.'member';

		$this->_info[$membertype] = false;
		$this->_origin[$membertype] = false;

		$where = array();
		$where['category'] = $type;
		$where['usertype'] = 'user';
		$where['userid'] = $this->_info['userfeatures']['id'];

		if(is_object($this->_queuemember) === false
		|| ($module = &$this->_service->get_module($type.'features')) === false
		|| ($this->_info[$membertype] = $this->_queuemember->get_all_where($where)) === false
		|| ($nb = count($this->_info[$membertype])) === 0)
		{
			$this->_info[$membertype] = false;
			return(false);
		}

		$ref = &$this->_info[$membertype];

		for($i = 0;$i < $nb;$i++)
		{
			if(($id = $module->get_primary(array('name' => $ref[$i]['queue_name']))) === false)
				continue;
			else if($type === 'queue')
				$ref[$i]['queuefeaturesid'] = $id;
			else
				$ref[$i]['groupfeaturesid'] = $id;
		}

		$this->_origin[$membertype] = $this->_queuemember->get_origin_list();

		return($this->_info[$membertype]);
	}

	function _get_voicemail($voicemailid=null)
	{
		$this->_info['voicemail'] = false;
		$this->_origin['voicemail'] = false;

		if ($voicemailid === null) {
			$voicemailid = (int) $this->_info['userfeatures']['voicemailid'];
		}

		if(is_object($this->_voicemail) === false
		|| $voicemailid === 0
		|| $this->_voicemail->get($voicemailid) === false)
			return(false);

		$this->_info['voicemail'] = $this->_voicemail->get_info('voicemail');

		$this->_origin['voicemail'] = $this->_voicemail->get_origin('voicemail');

		return($this->_info['voicemail']);
	}

	function _get_agentfeatures()
	{
		$this->_info['agentfeatures'] = false;
		$this->_origin['agentfeatures'] = false;

		if(is_object($this->_agentfeatures) === false
		|| $this->_agentfeatures->get($this->_info['userfeatures']['agentid']) === false)
			return(false);

		$this->_origin['agentfeatures'] = $this->_agentfeatures->get_origin();

		return($this->_info['agentfeatures']);
	}

	function _get_phonefunckey()
	{
		$this->_info['phonefunckey'] = false;
		$this->_origin['phonefunckey'] = false;

		if(is_object($this->_phonefunckey) === false)
			return(false);

		$func_key = $this->get_phonefunckey_identity(false);

		$this->_info['phonefunckey'] = $func_key;
		$this->_origin['phonefunckey'] = $this->_phonefunckey->get_origin_list();

		return($this->_info['phonefunckey']);
	}

	function get_timezones()
	{
		if(is_object($this->_voicemail) === false)
			return(false);

		return($this->_voicemail->get_timezones());
	}

	function get_phonefunckey_type()
	{
		return($this->_phonefunckey->get_type_list());
	}

	function get_bsfilter_list()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);

		$rs = $this->_dso->get_bsfilter($this->_info['userfeatures']['id'],
						$this->_info['userfeatures']['bsfilter']);

		if($rs === false || ($nb = count($rs)) === 0)
			return(false);

		for($i = 0;$i < $nb;$i++)
		{
			$ref = &$rs[$i];
			$ref = $this->_userfeatures->_prepare($ref);
			$ref['callfilteridentity'] = $ref['callfiltername'].' / '.$ref['identity'];
		}

		if(empty($rs) === true)
			$rs = false;

		return($rs);
	}

	function get_config_import_file()
	{
		return($this->_chk_exists_cfg('import','file',true));
	}

	function _get_config_file_maxsize()
	{
		if(($import_file = $this->_chk_exists_cfg('import','file',true)) !== false
		&& isset($import_file['size']) === true)
			return(dwho_get_memory($import_file['size'],false));

		return(false);
	}

	function get_voicemail_list()
	{
		if(is_object($this->_voicemail) === false)
			return(false);

		$order = array();
		$order['fullname'] = SORT_ASC;
		$order['mailbox'] = SORT_ASC;

		return($this->_voicemail->get_voicemail_list(null,$order));
	}

	function get_agent_list()
	{
		if(is_object($this->_agentfeatures) === false)
			return(false);

		$order = array();
		$order['firstname'] = SORT_ASC;
		$order['lastname'] = SORT_ASC;
		$order['number'] = SORT_ASC;

		return($this->_agentfeatures->get_all(null,true,$order));
	}

	function get_profileclient_list()
	{
		if(empty($this->_profileclientlist) === false)
			return($this->_profileclientlist);
		elseif($this->_cti_profile === false
		|| ($list = $this->_cti_profile->find_all()) === false
		|| ($nb = count($list)) === 0)
			return(false);

		$this->_profileclientlist = array();

		for($i=0;$i<$nb;$i++)
		{
			$ref = &$list[$i];
			$this->_profileclientlist[$ref['id']] = $ref['name'];
		}

		return($this->_profileclientlist);
	}

	function get_elements()
	{
		$r = array();

		$r['userfeatures'] = $this->_userfeatures->get_element();
		$r['linefeatures'] = $this->_linefeatures->get_element();
		$r['dialaction'] = $this->_dialaction->get_element();

		if(is_object($this->_queuemember) === true)
			$r['qmember'] = $this->_queuemember->get_element();
		else
			$r['qmember'] = false;

		if(is_object($this->_voicemail) === true
		&& ($vmelement = $this->_voicemail->get_elements()) !== false)
			$r = array_merge($vmelement,$r);
		else
			$r['voicemail'] = false;

		if(is_object($this->_phonefunckey) === true)
			$r['phonefunckey'] = $this->_phonefunckey->get_element();
		else
			$r['phonefunckey'] = false;

		return($r);
	}

	function get_users_search($search,$disable=null,$order=null,$limit=null,$assoc=false)
	{
		return($this->_get_users('search',$disable,$search,$order,$limit,$assoc));
	}

	function get_users_search_number($search,$disable=null,$order=null,$limit=null,$assoc=false)
	{
		return($this->_get_users('search_number',$disable,$search,$order,$limit,$assoc));
	}

	function get_users_list($disable=null,$order=null,$limit=null,$assoc=false,$initialized=null)
	{
		return($this->_get_users('list',$disable,null,$order,$limit,$assoc,$initialized));
	}

	function _get_users($action,$disable=null,$search='',$order=null,$limit=null,$assoc=false,$initialized=null)
	{
		$search = strval($search);

		switch($action)
		{
			case 'search':
				$rs = $this->_dso->search($search,$disable,null,$order,$limit);
				break;
			case 'search_number':
				$rs = $this->_dso->search($search,$disable,true,$order,$limit);
				break;
			case 'list':
			default:
				$rs = $this->_dso->get_all($disable,$initialized,$order,$limit);
		}

		$this->_cnt = $this->_dso->get_cnt();

		if($rs === false || ($nb = count($rs)) === 0)
			return(false);

		$r = array();

		for($i = 0;$i < $nb;$i++)
		{
			$ref = &$rs[$i];

			$userfeatures = dwho_array_startswith_key($ref,'userfeatures.',true);
			$r[$userfeatures['id']] = $this->_userfeatures->_prepare($userfeatures);
			if($this->_components === false)
				continue;

			$entity = dwho_array_startswith_key($ref,'entity.',true);
			$r[$userfeatures['id']]['entity'] = $this->_entity->_prepare($entity);
			$r[$userfeatures['id']]['masterline'] = $this->_linefeatures->get_user_masterline($userfeatures['id']);
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

	function get_phonefunckey_result()
	{
		$result = $this->get_result('phonefunckey');

		if(is_array($result) === false
		|| ($nb = count($result)) === 0)
			return(null);

		$r = array();

		for($i = 0;$i < $nb;$i++)
			$r[$i] = $this->_phonefunckey->_prepare($result[$i]);

		return($r);
	}

	function get_destination_list()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			$id = 0;
		else
			$id = $this->_info['userfeatures']['id'];

		$this->_get_destination();

		$r = $this->_destination;
		$r['users'] = $this->get_destination_users($id);
		$r['groups'] = $this->get_destination_groups();
		$r['queues'] = $this->get_destination_queues();
		$r['meetme'] = $this->get_destination_meetme();
		$r['voicemail'] = $this->get_destination_voicemail();
		$r['schedule'] = $this->get_destination_schedule();
		$r['sounds'] = $this->_dialaction->get_sound();
		$r['outcalls']  = $this->get_destination_outcall();
		//$r['trunk'] = $this->get_destination_trunk();

		return($r);
	}

	function get_phonefunckey_identity($result=true)
	{
		if (dwho_issa('userfeatures',$this->_info) === false)
			return(false);
		$res = array();
		if (($fks_v1 = $this->_get_phonefunckey_identity_v1($this->_info['userfeatures']['id'], $result)))
			$res = $res + $fks_v1;
		if (($fks_v2 = $this->_get_phonefunckey_identity_v2($this->_info['userfeatures']['id'], $result)))
			$res = $res + $fks_v2;
		ksort($res);
		return(array_values($res));
	}

	private function _get_phonefunckey_identity_v1($user_id, $result=true)
	{
		if(($rs = $this->_dso->get_phonefunckey($user_id)) === false
		|| ($nb = count($rs)) === 0)
			return(false);

		$r = array();

		for($i = 0;$i < $nb;$i++)
		{
			$ref = &$rs[$i];
			$phonefunckey = $this->_process_func_key($ref);
			$idx = $phonefunckey['fknum'];
			$res = $this->_process_func_key_dest($phonefunckey, $ref);
			if ($result)
				$r[$idx] = array('phonefunckey' => $phonefunckey, 'result' => $res);
			else
				$r[$idx] = $phonefunckey;
		}

		if(empty($r) === true)
			return(false);

		return($r);
	}

	private function _get_phonefunckey_identity_v2($user_id, $result=true)
	{
		if(($rs = $this->_dso->get_phonefunckey_v2($this->_info['userfeatures']['func_key_private_template_id'])) === false
		|| ($nb = count($rs)) === 0)
			return(false);

		$r = array();

		for($i = 0;$i < $nb;$i++)
		{
			$ref = &$rs[$i];

			$ref['phonefunckey.iduserfeatures'] = $user_id;
			$ref['phonefunckey.progfunckey'] = false;
			$ref['phonefunckey.supervision'] = dwho_bool($ref['phonefunckey.supervision']);

			$phonefunckey = $this->_process_func_key($ref);
			$idx = $phonefunckey['fknum'];
			$res = $this->_process_func_key_dest($phonefunckey, $ref);
			if ($result)
				$r[$idx] = array('phonefunckey' => $phonefunckey, 'result' => $res);
			else
				$r[$idx] = $phonefunckey;
		}

		if(empty($r) === true)
			return(false);

		return($r);
	}

	private function _process_func_key($ref)
	{
		return $this->_phonefunckey->_prepare(dwho_array_startswith_key($ref,'phonefunckey.',true));
	}

	private function _process_func_key_dest($phonefunckey, $ref)
	{
		$res = null;
		switch($phonefunckey['type'])
		{
			case 'user':
				$res = dwho_array_startswith_key($ref,'userfeatures.',true);
				$res['identity'] = $this->_service->identity('userfeatures',$res,false);
				break;
			case 'group':
				$res = dwho_array_startswith_key($ref,'groupfeatures.',true);
				$res['identity'] = $this->_service->identity('groupfeatures',$res,false);
				break;
			case 'queue':
				$res = dwho_array_startswith_key($ref,'queuefeatures.',true);
				$res['identity'] = $this->_service->identity('queuefeatures',$res,false);
				break;
			case 'meetme':
				$res = dwho_array_startswith_key($ref,'meetmefeatures.',true);
				$res['identity'] = $this->_service->identity('meetmefeatures',$res,false);
				break;
			case 'extenfeatures-agentstaticlogin':
			case 'extenfeatures-agentstaticlogoff':
			case 'extenfeatures-agentstaticlogtoggle':
				$res = dwho_array_startswith_key($ref,'agentfeatures.',true);
				$res['identity'] = $this->_service->identity('agentfeatures',$res,false);
				break;
			case 'extenfeatures-paging':
				$res = dwho_array_startswith_key($ref,'paging.',true);
				$res['identity'] = $this->_service->identity('paging',$res,false);
		}
		return($res);
	}

	function _get_destination()
	{
		if(is_array($this->_destination) === true)
			return(true);

		$this->_destination = array();

		return(true);
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
		|| dwho_issa('userfeatures',$arr) === false)
			return(false);

		$arr['userfeatures']['callerid'] = $this->_service->mk_callerid($arr['userfeatures']['callerid']);

		$this->set_userfeatures($action,$arr['userfeatures']);

		$vmaction = $action;

		if(isset($arr['voicemail-option']) === true)
		{
			switch($arr['voicemail-option'])
			{
				case 'add':
				case 'none':
					$vmaction = $arr['voicemail-option'];
					$arr['userfeatures']['voicemailid'] = null;
					break;
				case 'search':
					$vmaction = 'edit';
					$this->_get_voicemail((int) $arr['userfeatures']['voicemailid']);
					break;
			}
		}

		switch($this->get_result_var('userfeatures','voicemailtype'))
		{
			case 'asterisk':
			break;
			default:
				$vmaction = 'none';
				$this->_set_result_var('userfeatures','voicemailid',null);
			break;
		}

		if(dwho_issa('linefeatures',$arr) === true
		&& $this->set_linefeatures($arr['linefeatures']) !== false
		&& isset($this->_result['linefeatures'][0]))
		{
			$protocolname = $this->_result['linefeatures'][0]['protocol'];
			$name = $this->_result['linefeatures'][0]['name'];
			$number = $this->_result['linefeatures'][0]['number'];
			$context = $this->_result['linefeatures'][0]['context'];

			if(dwho_issa('voicemail',$arr) === true)
			{
				if($vmaction === 'add')
					$arr['voicemail']['context'] = $context;

				$arr['voicemail']['locale'] = $this->get_result_var('userfeatures','language');

				if(isset($arr['voicemail']['deletevoicemail']) === false)
					$arr['voicemail']['deletevoicemail'] = 0;

				if($vmaction !== 'disable')
					$this->set_voicemail($vmaction,$arr['voicemail']);

				$this->_set_result('mailbox', $arr['voicemail']['mailbox'].'@'.$context);
			}
			else
			{
				$this->_set_result_var('userfeatures','voicemailtype',null);
				$this->_set_result_var('userfeatures','enablevoicemail',false);
				$this->_set_result_var('userfeatures','voicemailid',null);
			}

			if(dwho_issa('group',$arr) === true
			&& dwho_issa('group-select',$arr) === true)
			{
				$this->set_groupmember($arr['group'],
						$arr['group-select'],
						$name,
						$context,
						$number,
						$protocolname);
			}

			if(dwho_issa('queue',$arr) === true
			&& dwho_issa('queue-select',$arr) === true)
			{
				$this->set_queuemember($arr['queue'],
							$arr['queue-select'],
							$name,
							$context,
							$number,
							$protocolname);
			}

			if(($rsexten = $this->chk_exten('exten',
											$number,
											$context,
											'user',
											0)) !== false) {
				$this->_set_result('exten',$rsexten);
			}
		}
		elseif(dwho_issa('voicemail',$arr) === true
		&& (isset($this->_info['linefeatures']) === false
			|| $this->_info['linefeatures'] === false))
		{
			$this->_set_error('voicemail', dwho_i18n::babelfish('user_need_lines_to_have_voicemail'));
			dwho_report::push('error', dwho_i18n::babelfish('user_need_lines_to_have_voicemail'));
			return(false);
		}

		if(dwho_issa('dialaction',$arr) === true)
			$this->set_dialaction($arr['dialaction']);

		if(dwho_issa('rightcall',$arr) === true)
			$this->set_rightcall($arr['rightcall']);

		if(dwho_issa('phonefunckey',$arr) === true)
			$this->set_phonefunckey($arr['phonefunckey']);

		if(array_key_exists('schedule_id', $arr))
			$this->_set_result('schedule_id', intval($arr['schedule_id']));

		if($this->get_errnb() > 0)
			$r = false;
		else
			$r = true;

		return($r);
	}

	function set_userfeatures($action,$arr)
	{
		if(($action !== 'add' && $action !== 'edit') === true
		|| ($action === 'edit' && dwho_issa('userfeatures',$this->_origin) === false) === true)
			return(false);
		else if(is_array($arr) === false)
		{
			$this->_set_result('userfeatures',null);
			$this->_set_error('userfeatures','invalid data');
			return(false);
		}

		if (isset($arr['enableclient'])
		&& (bool) $arr['enableclient'] === true
		&& (dwho_has_len($arr['loginclient']) === false
			|| dwho_has_len($arr['passwdclient']) === false))
		{
			dwho_report::push('error', 'cant_activate_xivoclient_without_login_and_password');
			$err = array('loginclient' => 'empty', 'passwdclient' => 'empty');
			$this->_set_error('userfeatures', $err);
		}

		$this->get_musiconhold();

		if(isset($arr['musiconhold']) === true
		&& isset($this->_musiconhold[$arr['musiconhold']]) === false)
			$arr['musiconhold'] = '';

		if($action === 'edit'
		&& isset($arr['cti_profile_id']) === false
		&& isset($arr['enableclient']) === false)
			$arr['cti_profile_id'] = $this->_origin['userfeatures']['cti_profile_id'];

		$this->get_profileclient_list();

		if(isset($arr['cti_profile_id']) === true
		&& isset($this->_profileclientlist[$arr['cti_profile_id']]) === false)
			$arr['cti_profile_id'] = '';

		if(isset($arr['destunc']) === true
		&& dwho_has_len($arr['destunc']) === false)
			$arr['enableunc'] = false;

		if(isset($arr['destrna']) === true
		&& dwho_has_len($arr['destrna']) === false)
			$arr['enablerna'] = false;

		if(isset($arr['destbusy']) === true
		&& dwho_has_len($arr['destbusy']) === false)
			$arr['enablebusy'] = false;

		if(isset($arr['outcallerid-type']) === true)
			$arr['outcallerid'] = $arr['outcallerid-type'];
		else if(isset($arr['outcallerid']) === false)
			$arr['outcallerid'] = '';

		if(isset($arr['outcallerid-custom']) === true
		&& $arr['outcallerid'] === 'custom')
			$arr['outcallerid'] = $arr['outcallerid-custom'];

		if($action === 'add')
		{
			$arr['func_key_template_id'] = null;
			$arr['func_key_private_template_id'] = 0;
		}
		else
		{
			$arr['func_key_private_template_id'] = $this->_info['userfeatures']['func_key_private_template_id'];
			$func_key_template_id = $this->_info['userfeatures']['func_key_template_id'];
			$arr['func_key_template_id'] = ($func_key_template_id == 0)?null:(int)$func_key_template_id;
		}

		if(($rs = $this->_userfeatures->chk_values($arr)) === false)
		{
			$this->_set_result('userfeatures',$this->_userfeatures->get_filter_result());
			$this->_set_error('userfeatures',$this->_userfeatures->get_filter_error());
			return(false);
		}
		else
		{
			if($action === 'edit')
				$id = $this->_origin['userfeatures']['id'];
			else
				$id = null;

			if($this->_userfeatures->loginclient_exists($rs['loginclient'],$id) === true)
			{
				$this->_set_result('userfeatures',$rs);
				$this->_set_error('userfeatures',array('loginclient' => 'client login exists'));
				return(false);
			}
		}

		if(dwho_has_len($rs['agentid']) === false
		|| is_object($this->_agentfeatures) === false
		|| $this->_agentfeatures->get_primary($rs['agentid']) === false)
			$rs['agentid'] = null;

		$this->_set_result('userfeatures',$rs);

		return(true);
	}

	function set_linefeatures($arr)
	{
		if(is_array($arr) === false
		|| $this->_linefeatures === false
		|| ($userfeatures = $this->get_result('userfeatures')) === null)
			return(false);

		$reslist = $error = array();
		$errnb = 0;

		foreach($arr as $key => $list)
		{
			for($i = 0;$i < count($list);$i++)
			{
				if (isset($arr[$key][$i]) === false
				|| $arr[$key][$i] === '')
					$reslist[$i][$key] = '';
				else
					$reslist[$i][$key] = $arr[$key][$i];
			}
		}

		if(empty($reslist) === true)
		{
			$this->_set_result('linefeatures',false);
			$this->_set_error('linefeatures','empty');
			return(false);
		}

		array_pop($reslist);

		if (empty($reslist) === true)
			return(null);

		for($i = 0;$i < count($reslist);$i++)
		{
			$ref = &$reslist[$i];

			$ref['protocolid'] = 0;
			if (isset($ref['id']) === true
			&& (int) $ref['id'] !== 0)
				$ref['id'] = (int) $ref['id'];

			if($ref['protocol'] === XIVO_SRE_IPBX_AST_PROTO_SCCP)
				$ref['name'] = $ref['number'];
			elseif(dwho_has_len($ref, 'name') === false)
				$ref['name'] = $this->gen_password(6,true);

			$protocolname = $ref['protocol'];
			$number = $ref['number'];

			if($protocolname === XIVO_SRE_IPBX_AST_PROTO_CUSTOM)
				$this->_set_result_var('userfeatures','voicemailtype',null);

			$ref['num'] = isset($ref['num']) ? (int) $ref['num'] : 1;
			$ref['callerid'] = $this->_service->mk_callerid($userfeatures['callerid'],$number,$protocolname);

			if($this->_linefeatures->chk_values($ref) === false)
			{
				$errnb++;
				$error[$i] = $this->_linefeatures->get_filter_error();
			}
		}

		$this->_set_result('linefeatures',$reslist);

		if ($errnb >> 0)
		{
			$this->_set_errors('linefeatures',$error);
			return(false);
		}

		return(true);
	}

	function set_voicemail($action,$voicemail)
	{
		if(($action !== 'add' && $action !== 'edit') === true
		|| is_array($voicemail) === false
		|| empty($voicemail) === true
		|| is_object($this->_voicemail) === false)
			return(false);

		$rs = array();
		$rs['voicemail'] = $voicemail;

		if($action === 'add')
			$r = $this->_voicemail->set_add($rs);
		else
			$r = $this->_voicemail->set_edit($rs,false);

		if(($voicemailerror = $this->_voicemail->get_error('voicemail')) !== null)
		{
			$this->_set_error('voicemail',$voicemailerror);
			$this->_set_result('voicemail',$this->_voicemail->get_result('voicemail'));
		}
		else
			$this->_set_result('voicemail',$this->_voicemail->get_result('voicemail'));

		return($r);
	}

	function set_phonefunckey($list)
	{
		if(($list = dwho_group_array('fknum',$list)) === false)
			return(false);

		$reslist = $errlist = $tmp = array();
		$bsfilter = $this->get_result_var('userfeatures','bsfilter');

		$nb = count($list);

		for($i = $j = 0;$i < $nb;$i++)
		{
			$ref = &$list[$i];
			$ref['iduserfeatures'] = 0;

			if(($val = $this->_phonefunckey->mk_values_array($ref)) === false)
				continue;
			else if(($info = $this->_phonefunckey->chk_values($val)) === false)
			{
				$errlist[$j] = $this->_phonefunckey->get_filter_error();
				$reslist[$j++] = $this->_phonefunckey->get_filter_result();
			}
			else if($info['typeextenumbers'] === 'extenfeatures'
			&& $info['typevalextenumbers'] === 'bsfilter'
			&& $bsfilter === 'no')
			{
				continue;
			}
			else if(isset($tmp[$info['fknum']]) === true)
			{
				$errlist[$j] = 'wrong phonefunckey';
				$reslist[$j++] = $info;
			}
			else
			{
				$tmp[$info['fknum']] = 1;
				$reslist[$j++] = $info;
			}

			if(($info['typeextenumbers'] !== null
			|| $info['typeextenumbersright'] !== null) === true
			&& $this->_phonefunckey->is_valid($info['typeextenumbers'],
							  $info['typevalextenumbers'],
							  $info['typeextenumbersright'],
							  $info['typevalextenumbersright'],
							  $info['iduserfeatures'],
							  null) === false)
				return(false);
		}

		$this->_set_result('phonefunckey',$reslist);

		if(empty($errlist) === false)
		{
			$this->_set_error('phonefunckey',$errlist);
			return(false);
		}

		return(true);
	}

	function set_groupmember($group,$select,$name,$context='',$number='',$protocolname='')
	{
		return($this->_set_member_type('group',$group,$select,$name,$context,$number,$protocolname));
	}

	function set_queuemember($queue,$select,$name,$context='',$number='',$protocolname='')
	{
		return($this->_set_member_type('queue',$queue,$select,$name,$context,$number,$protocolname));
	}

	function _set_member_type($type,$list,$select,$name,$context='',$number='',$protocolname='')
	{
		if(($type !== 'group' && $type !== 'queue') === true
		|| is_array($list) === false
		|| is_array($select) === false
		|| ($module = &$this->_service->get_module($type.'features')) === false
		|| is_object($this->_queuemember) === false)
			return(false);

		$slt = array_values($select);

		if(($nb = count($slt)) === 0)
			return(null);

		$info = array();
		$info['usertype'] = 'user';
		$info['userid'] = 0;
		$info['position'] = 0;
		$info['category'] = $type;

		$key = $type.'member';

		$tmp = $reslist = array();

		if($type === 'group')
			$info['penalty'] = 0;

		for($i = 0;$i < $nb;$i++)
		{
			$qname = &$slt[$i];

			if(isset($tmp[$qname]) === true
			|| isset($list[$qname]) === false
			|| ($chantype = dwho_ak('chantype',$list[$qname],true)) === false
			|| ($interface = $this->mk_interface($name,
							     $chantype,
							     $context,
							     $number,
							     $protocolname)) === false
			|| ($typeid = $module->get_primary(array('name' => $qname))) === false) {
				continue;
			}

			$info['queue_name'] = $qname;
			$info['interface'] = $interface['interface'];
			$info['channel'] = $interface['channel'];

			$values = array_merge($list[$qname],$info);

			if(($rs = $this->_queuemember->chk_values($values)) === false)
				continue;
			else if($type === 'queue')
				$rs['queuefeaturesid'] = $typeid;
			else
				$rs['groupfeaturesid'] = $typeid;

			$tmp[$qname] = 1;
			$reslist[] = $rs;
		}

		if(isset($reslist[0]) === false)
		{
			$this->_set_result($key,false);
			$this->_set_error($key,'empty');

			return(false);
		}

		$this->_set_result($key,$reslist);

		return(true);
	}

	function _set_exten($action,$user_id)
	{
		$rs = false;

		if(($user_id = dwho_ulongint($user_id)) === 0
		|| ($rs = $this->get_result('exten')) === null
		|| $rs === false)
			return($rs);
		else if(($rsexten = $this->set_exten($action,
											'exten',
											$rs['exten'],
											$rs['context'],
											'user',
											$user_id)) === false)
			return(false);

		$this->_set_result('exten',$rsexten);

		return(true);
	}

	function add()
	{
		$r = true;

		$this->_status = 'add';
		$this->_return = array();
		$user_id = null;

		if($this->get_errnb() > 0
		|| $this->add_func_key_private_tpl() === false
		|| ($user_id = $this->add_userfeatures()) === false
		|| $this->add_func_key_dest_user($user_id) === false
		|| ($line = $this->edit_linefeatures($user_id)) === false
		|| $this->add_phonefunckey($user_id) === false
		|| $this->add_rightcall($user_id) === false
		|| $this->add_dialaction($user_id) === false
		|| $this->add_schedule_path($user_id) === false)
		{
			$r = false;
			if($user_id !== null)
				$this->_reverse_add();
		}

		if($r === true && $line)
		{
			$line_id = $line[0]['id'];
			if($this->add_groupmember($user_id) === false
			|| $this->add_queuemember($user_id) === false
			|| $this->add_voicemail($user_id) === false
			|| ($extension_id = $this->add_exten($user_id)) === false
			|| $this->add_user_line($user_id, $line_id, $extension_id) === false
			|| $this->add_device_config($line_id) === false)
				$r = false;
		}

		if($r === false && $user_id !== null)
			$this->_reverse_add();

		$this->_status = '';

		return($r);
	}

	function add_func_key_private_tpl()
	{
		if(($rs = $this->get_result('userfeatures')) === null
		|| $rs === false)
			return(false);

		if(($func_key_template_id = $this->_func_key_manager->add_private_tpl($rs['firstname'].' '.$rs['lastname'])) === false)
			return(false);

		$this->_return['func_key_private_template_id'] = $func_key_template_id;
		$this->_set_result_var('userfeatures','func_key_private_template_id', $func_key_template_id);
		return(true);
	}

	function add_userfeatures()
	{
		if(($rs = $this->get_result('userfeatures')) === null
		|| $rs === false)
			return(false);

		$this->_return['userfeatures'] = $this->_userfeatures->add($rs);

		return($this->_return['userfeatures']);
	}

	function add_func_key_dest_user($user_id)
	{
		if (!($func_key_id = $this->_func_key_manager->add_dst_for_user($user_id)))
			return(false);

		$this->_return['func_key_dest_user'] = $user_id;
		return(true);
	}

	function add_user_line($user_id, $line_id)
	{
		if(($user_id = (int) $user_id) === 0
		|| ($line_id = (int) $line_id) === 0
		|| ($extension = $this->_extensions->get_loaded_exten($this->_return['exten'])) === false)
			return(false);

		$rs = array();
		$rs['user_id'] = $user_id;
		$rs['line_id'] = $line_id;
		$rs['extension_id'] = $extension[0]['id'];
		$rs['main_line'] = true;
		$rs['main_user'] = true;

		return(($this->_return['user_line'] = $this->_user_line->add($rs)));
	}

	function add_device_config($line_id)
	{
		if($this->_provddevice === false
		|| ($rs = $this->get_result('linefeatures')) === null
		|| $rs === false)
			return(false);

		$new_device = $rs[0];

		// new device associated
		if (empty($new_device['device']) === false)
		{
			$vm_number = (string) $this->get_result_var('voicemail', 'mailbox');

			if (($device = $this->_device->get($new_device['device'])) === false)
			{
				dwho_report::push('error', 'device doesn\'t exist');
				return(false);
			}
			elseif($this->_provddevice->update_config_from_line($device['device']['id'], $line_id) === false)
			{
				dwho_report::push('error', 'Can\'t edit device');
				return(false);
			}
			elseif ($new_device['protocol'] === XIVO_SRE_IPBX_AST_PROTO_SCCP
			&& $this->_sccpdevice->init_device($device['device']['mac'],$new_device['name'],$vm_number) === false)
			{
				dwho_report::push('error', 'Can\'t initialize SCCP device');
				return(false);
			}
		}
		return(true);
	}

	function add_voicemail($user_id)
	{
		if(($voicemailid = dwho_ulongint($this->get_result_var('userfeatures','voicemailid'))) === 0)
			return($this->_add_voicemail($user_id));
		else if(is_object($this->_voicemail) === false
		|| $this->_voicemail->get($voicemailid) === false
		|| ($rs = $this->get_result('voicemail')) === null
		|| $rs === false)
			return($rs);

		return(($this->_return['voicemail'] = $this->_voicemail->edit(false)));
	}

	function _add_voicemail($user_id)
	{
		if(($user_id = dwho_ulongint($user_id)) === 0
		|| ($rs = $this->get_result('voicemail')) === null
		|| $rs === false)
			return($rs);
		else if(($this->_return['voicemail'] = $this->_voicemail->add()) === true
		&& (($voicemailid = dwho_ulongint($voicemailid = $this->_voicemail->get_return('voicemail'))) === 0
		   || $this->_userfeatures->edit($user_id,array('voicemailid' => $voicemailid)) === false) === true)
			return(false);

		return($this->_return['voicemail']);
	}

	function add_phonefunckey($user_id)
	{
		$rs = false;

		if(($user_id = dwho_ulongint($user_id)) === 0
		|| ($rs = $this->get_result('phonefunckey')) === null
		|| $rs === false)
			return($rs);

		$id = $this->_return['phonefunckey'] = array();
		$id['iduserfeatures'] = $user_id;


		$nb = count($rs);
		for($i = 0;$i < $nb;$i++)
		{
			$ref = &$rs[$i];
			$ref['iduserfeatures'] = $user_id;

			if(($destination = $this->_determine_destination($ref)) === false) {
				if($this->_phonefunckey->add($ref) === false)
					return(false);
			} else {
				if($this->_add_func_key_v2($destination, $ref) === false)
					return(false);
			}

			$id['fknum'] = $ref['fknum'];
			$this->_return['phonefunckey'][] = $id;
		}

		if(isset($this->_return['phonefunckey'][0]) === false)
			$this->_return['phonefunckey'] = false;

		return(true);
	}

	private function _determine_destination($ref)
	{
		if($ref['typeextenumbersright'] === 'user' && $ref['typevalextenumbers'] !== 'bsfilter')
		{
			return('user');
		}
		elseif($ref['typeextenumbersright'] === 'group')
		{
			return('group');
		}
		elseif($ref['typeextenumbersright'] === 'queue')
		{
			return('queue');
		}
		elseif($ref['typeextenumbersright'] === 'meetme')
		{
			return('conference');
		}
		elseif($ref['typeextenumbers'] === 'extenfeatures'
			  && in_array($ref['typevalextenumbers'], $this->_forward_services)
		)
		{
			return('forward');
		}
		elseif($ref['typeextenumbers'] === 'extenfeatures'
				&& in_array($ref['typevalextenumbers'], $this->_valid_services)
		)
		{
			return('service');
		}
		elseif($ref['typevalextenumbers'] == 'parkpos')
		{
			return('park_position');
		}
		elseif($ref['typeextenumbers'] == 'featuremap' || $ref['typeextenumbers'] == 'generalfeatures')
		{
			return('features');
		}
		elseif($ref['typevalextenumbers'] == 'paging')
		{
			return('paging');
		}
		elseif( $ref['typeextenumbers'] === 'extenfeatures'
				&& $this->_func_key_agent->action_exists($ref['typevalextenumbers']))
		{
			return('agent');
		}
		elseif($ref['typevalextenumbers'] == 'bsfilter')
		{
			return('bsfilter');
		}
		elseif( ($ref['typeextenumbers'] === null || $ref['typeextenumbers'] === '')
				&& ($ref['typevalextenumbers'] === null || $ref['typevalextenumbers'] === ''))
		{
			return('custom');
		}
		return(false);
	}

	private function _determine_func_key_type($ref)
	{
		if($ref['typeextenumbers'] === 'featuremap')
		{
			return('dtmf');
		}
		elseif($ref['typevalextenumbers'] == 'parkext')
		{
			return('forward');
		}
		return('speeddial');
	}

	private function _add_func_key_v2($destination, $data)
	{
		$type = $this->_determine_func_key_type($data);

		switch($destination) {
		case 'user':
		case 'group':
		case 'queue':
		case 'conference':
		case 'paging':
			$value = $data['typevalextenumbersright'];
			$func_key_id = $this->_func_key_manager->get_fk_id_with_dst($destination, $value);
			break;
		case 'service':
			$service_type = $data['typevalextenumbers'];
			$extension_id = $this->_func_key_manager->get_dst_id_for_extension($service_type);
			$func_key_id = $this->_func_key_manager->get_fk_id_with_dst($destination, $extension_id);
			break;
		case 'forward':
			$forward_type = $data['typevalextenumbers'];
			$extension_id = $this->_func_key_manager->get_dst_id_for_extension($forward_type);
			$number = $data['exten'];
			$func_key_id = $this->_func_key_manager->add_dst_for_forward($extension_id, $number);
			break;
		case 'park_position':
			$park_position = $data['exten'];
			$func_key_id = $this->_func_key_manager->add_dst_for_park_position($park_position);
			break;
		case 'features':
			$feature_type = $data['typevalextenumbers'];
			$func_key_id = $this->_func_key_manager->get_dst_id_for_features($feature_type);
			break;
		case 'custom':
			$value = $data['exten'];
			$func_key_id = $this->_func_key_manager->add_dst_for_custom($value);
			break;
		case 'agent':
			$agent_id = $data['typevalextenumbersright'];
			$action = $data['typevalextenumbers'];
			$func_key_id = $this->_func_key_manager->get_dst_id_for_agent($agent_id, $action);
			break;
		case 'bsfilter':
			$filtermember_id = $this->_find_filtermember_id($data);
			$func_key_id = $this->_func_key_manager->get_fk_id_with_dst($destination, $filtermember_id);
			break;
		default:
			dwho_report::push('error', "func key of type '".$destination."' is unknown");
			return(false);
		}

		if($func_key_id === false) {
			dwho_report::push('error', 'could not find func key for destination '.$destination);
			return(false);
		}

		$fk = array();
		$fk['func_key_id'] = $func_key_id;
		$fk['type'] = $type;
		$fk['position'] = (int) $data['fknum'];
		$fk['blf'] = (bool) $data['supervision'];
		$fk['label'] = (string) $data['label'];
		$fk['template_id'] = (int) $this->get_result_var('userfeatures', 'func_key_private_template_id');

		return($this->_func_key_manager->add_fk_mapping($fk));
	}

	private function _find_filtermember_id($data)
	{
		if($this->_is_boss($data['iduserfeatures'])) {
			$boss_id = $data['iduserfeatures'];
			$secretary_id = $data['typevalextenumbersright'];
		} else {
			$boss_id = $data['typevalextenumbersright'];
			$secretary_id = $data['iduserfeatures'];
		}

		$callfiltermember = $this->_callfiltermember->get_by_boss_secretary($boss_id, $secretary_id);
		if($callfiltermember === false) {
			dwho_report::push('error', 'could not find boss or secretary member');
			return(false);
		}

		return $callfiltermember['id'];
	}

	private function _is_boss($user_id)
	{
		$bosses = $this->_callfilter->get_boss_users();
		if ($bosses === false)
			return(false);

		foreach($bosses as $boss) {
			if($boss['id'] == $user_id)
				return(true);
		}

		return(false);
	}

	function add_schedule_path($user_id)
	{
		return $this->_schedule_path->schedule_add(
			$this->get_result('schedule_id'),
			'user',
			$user_id);
	}

	function add_exten($user_id)
	{
		if(($rs = $this->_set_exten('add',$user_id)) !== true)
			return($rs);

		return(parent::add_exten('exten'));
	}

	function add_groupmember($user_id)
	{
		return($this->_add_member_type('group',$user_id));
	}

	function add_queuemember($user_id)
	{
		return($this->_add_member_type('queue',$user_id));
	}

	function _add_member_type($type,$user_id)
	{
		if(($type !== 'group' && $type !== 'queue') === true
		|| ($user_id = dwho_ulongint($user_id)) === 0)
			return(false);

		$key = $type.'member';

		if(($rs = $this->get_result($key)) === null || $rs === false)
			return($rs);

		$ret = $this->_return[$key] = array();

		$nb = count($rs);
		for($i = 0;$i < $nb;$i++)
		{
			$rs[$i]['userid'] = $user_id;

			unset($rs[$i]['groupfeaturesid'],$rs[$i]['queuefeaturesid']);

			if($this->_queuemember->add($rs[$i]) === false)
				return(false);

			$ret['queue_name'] = $rs[$i]['queue_name'];
			$ret['interface'] = $rs[$i]['interface'];

			$this->_return[$key][] = $ret;
		}

		if(isset($this->_return[$key][0]) === false)
			$this->_return[$key] = false;

		return(true);
	}

	function _reverse_add()
	{
		if($this->_status !== 'add'
		|| is_array($this->_return) === false
		|| empty($this->_return) === true)
			return(false);

		if(array_key_exists('func_key_dest_user', $this->_return)) {
			$this->_func_key_manager->delete_dst_for_user($this->_return['func_key_dest_user']);
		}

		foreach($this->_return as $key => $val)
		{
			if($val === false)
				continue;

			switch($key)
			{
				case 'userfeatures':
					$this->_userfeatures->delete($val);
					break;
				case 'rightcall':
					$this->_rightcall->reverse_add();
					break;
				case 'dialaction':
					$this->_dialaction->reverse_add();
					break;
				case 'phonefunckey':
					if(is_array($val) === false)
						continue;

					foreach($val as $phonefkeyval)
					{
						if($phonefkeyval !== false)
							$this->_phonefunckey->delete($phonefkeyval);
					}
					break;
				case 'voicemail':
					$this->_voicemail->reverse_add();
					break;
				case 'exten':
					$this->_extensions->delete_exten($val);
					break;
				case 'groupmember':
				case 'queuemember':
					if(is_array($val) === false)
						continue;

					foreach($val as $memberval)
						$this->_queuemember->delete($memberval);
					break;
				case 'user_line':
					$this->_user_line->delete($val);
					break;
			}
		}

		if(array_key_exists('func_key_private_template_id', $this->_return)) {
			$this->_func_key_manager->delete_template($this->_return['func_key_private_template_id']);
		}

		$this->_line->_status = 'add';
		$this->_line->_reverse_add();

		return(true);
	}

	function edit()
	{
		$this->_status = 'edit';
		$this->_return = array();
		$user_id = (int) $this->_info['userfeatures']['id'];

		if(is_array($this->_info) === false
		|| empty($this->_info) === true
		|| $this->get_errnb() > 0
		|| $this->edit_userfeatures() === false
		|| $this->edit_phonefunckey() === false
		|| $this->edit_rightcall() === false
		|| $this->edit_dialaction() === false
		|| $this->edit_schedule_path() === false)
		{
			$this->_reverse_edit();
			$this->_status = '';
			return(false);
		}

		if(($line = $this->get_result('linefeatures')) !== null
		&& dwho_issa(0, $line) === true
		&& ($existingline = $this->edit_linefeatures($user_id)) !== false
		&& $this->edit_voicemail() !== false)
		{
			if(isset($this->_info['linefeatures'][0])
			&& $this->_info['linefeatures'][0]['number'] !== '')
			{
				if($this->edit_exten($user_id) === false
				|| $this->edit_user_line($user_id, $this->_info['linefeatures'][0]['id']) === false
				|| $this->edit_device_config() === false
				|| $this->edit_groupmember($user_id) === false
				|| $this->edit_queuemember($user_id) === false)
				{
					$this->_reverse_edit();
					$this->_status = '';
					return(false);
				}
			}
			else if($this->add_exten($user_id) === false
			|| ($line_id = $this->_return['linefeatures'][0]['id']) === 0
			|| $this->add_user_line($user_id, $line_id) === false
			|| $this->add_device_config($line_id) === false
			|| $this->add_groupmember($user_id) === false
			|| $this->add_queuemember($user_id) === false)
			{
				$this->_reverse_edit();
				$this->_status = '';
				return(false);
			}
		}
		elseif($this->get_result('linefeatures') === null
		&& $this->_info['linefeatures'] !== false)
		{
			if(isset($this->_info['userfeatures']['voicemailid'])
			&& (int) $this->_info['userfeatures']['voicemailid'] !== 0)
			{
				$this->_set_error('voicemail', dwho_i18n::babelfish('user_no_remove_lines_when_voicemail'));
				dwho_report::push('error', dwho_i18n::babelfish('user_no_remove_lines_when_voicemail'));
				return(false);
			}

			if ($this->delete_device_config() === false
			|| $this->delete_linefeatures() === false
			|| $this->delete_exten('exten') === false)
			{
				$this->_reverse_edit();
				$this->_status = '';
				return(false);
			}
		}

		$this->_status = '';
		return(true);
	}

	function edit_userfeatures()
	{
		if(dwho_issa('userfeatures',$this->_info) === false
		|| ($rs = $this->get_result('userfeatures')) === null
		|| $rs === false)
			return(false);

		$return = $this->_userfeatures->edit($this->_info['userfeatures']['id'],$rs);

		return(($this->_return['userfeatures'] = $return));
	}

	function edit_user_line($user_id, $linefeaturesid)
	{
		if(($user_id = (int) $user_id) === 0
		|| ($linefeaturesid = (int) $linefeaturesid) === 0
		|| ($extensionid = (int) $this->_info['exten']['id']) === 0)
			return(false);

		$rs = array();
		$rs['user_id'] = $user_id;
		$rs['line_id'] = $linefeaturesid;
		$rs['main_line'] = 'true';
		$rs['main_user'] = 'true';
		$where = $rs;
		$rs['extension_id'] = $extensionid;
		$rs['main_line'] = true;
		$rs['main_user'] = true;

		return(($this->_return['user_line'] = $this->_user_line->edit_where($where, $rs)));
	}

	function edit_linefeatures($user_id)
	{
		if(($rs = $this->get_result('linefeatures')) === null
		|| (($nbrs = count($rs)) === 0
			&& $this->has_existing_line() === false))
			return(null);

		if(is_null($rs) === true)
			$rs = array();

		$res = array();
		if (isset($this->_info['linefeatures']) === true
		&& ($info = $this->_info['linefeatures']) !== false
		&& ($nbinfo = count($info)) !== 0)
		{
			for($i = 0;$i < $nbinfo;$i++)
			{
				$ref = &$info[$i];
				$res[$ref['id']] = $ref;
			}
		}

		$return = $error = array();
		$errnb = 0;
		$linefeatures = array();
		$linefeatures['list'] = $res;
		$linefeatures['slt'] = dwho_array_intersect_key($rs,$linefeatures['list'],'id');
		$linefeatures['info'] = false;

		if($linefeatures['slt'] !== false)
		{
			$linefeatures['info'] = dwho_array_copy_intersect_key($rs,$linefeatures['slt'],'id');
			$linefeatures['list'] = dwho_array_diff_key($linefeatures['list'],$linefeatures['slt']);
		}

		if(empty($linefeatures['list']) === false)
		{
			$nb = count($linefeatures['list']);
			$linefeatures_list = array_values($linefeatures['list']);
			for($i = 0;$i < $nb;$i++)
			{
				$ref = &$linefeatures_list[$i];
				if (isset($ref['id']) === false)
					continue;
				if($this->_line->get($ref['id']) === false
				|| $this->_line->delete() === false)
				{
					dwho_report::push('error','Can\'t remove line: '.$ref['id'],'linefeatures');
					$this->_set_error('line',$this->_line->get_error());
					return(false);
				}
			}
		}

		for($i = 0;$i < $nbrs;$i++)
		{
			$line = &$rs[$i];

			$this->_line->_reset();
			$protocolname = $line['protocol'];
			$arr = array();

			if (isset($line['id']) === true
			&& (int) $line['id'] !== 0)
			{
				$methodset = 'set_edit';
				$method = 'edit';

				if (($existline = $this->_line->get($line['id'])) === false)
				{
					$errnb++;
					$error[$i] = 'line_not_exist';
					dwho_report::push('error',dwho_i18n::babelfish('Row '.($i+1).': line_not_exist'),'linefeatures');
					continue;
				}
			}
			else
			{
				$methodset = 'set_add';
				$method = 'add';
			}

			$arr['linefeatures'] =  $this->get_result_var('linefeatures', 0);

			$arr['user_id'] = $user_id;
			$arr['protocol'] = array();

			if ($protocolname === XIVO_SRE_IPBX_AST_PROTO_SIP)
				$arr['protocol']['call-limit'] = (2 * (int) $this->get_result_var('userfeatures', 'simultcalls'));
			$arr['protocol']['language'] = $this->get_result_var('userfeatures', 'language');
			$arr['protocol']['mailbox'] = $this->get_result('mailbox');
			$arr['protocol']['name'] = $line['name'];
			$arr['protocol']['context'] = $line['context'];

			if($protocolname === XIVO_SRE_IPBX_AST_PROTO_CUSTOM)
				$arr['protocol']['interface'] = $line['name'];
			elseif($protocolname === XIVO_SRE_IPBX_AST_PROTO_SCCP)
			{
				$arr['protocol']['cid_name'] = str_replace('"','',$line['callerid']);
				$arr['protocol']['cid_num'] = $line['number'];
			}

			if(($queue = $this->get_result('queue')) !== null
			&& ($queue_select = $this->get_result('queue-select')) !== null)
			{
				$arr['queue'] = $queue;
				$arr['queue-select'] = $queue_select;
			}
			if(($group = $this->get_result('group')) !== null
			&& ($group_select = $this->get_result('group-select')) !== null)
			{
				$arr['group'] = $group;
				$arr['group-select'] = $group_select;
			}

			if($this->_line->$methodset($arr,$protocolname) === false
			|| ($line_id = $this->_line->$method()) === false)
			{
				$errnb++;
				$error[$i] = $this->_line->get_error();
				$msgkey = array_shift(array_keys($this->_line->get_error()));
				$msg = array_shift($this->_line->get_error());
				if (empty($msgkey) === false)
					dwho_report::push('error','Line '.($i+1).': '.$msgkey.' '.$msg,'linefeatures');
				else
					dwho_report::push('error','Line '.($i+1).': unknow error during '.$method,'linefeatures');
			}
			$line['id'] = $line_id;
			$return[$i] = $line;
		}

		if ($errnb >> 0)
		{
			$this->_set_error('linefeatures',$error);
			return(false);
		}

		return(($this->_return['linefeatures'] = $return));
	}

	function edit_rightcall()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);

		return(parent::edit_rightcall($this->_info['userfeatures']['id']));
	}

	function edit_dialaction()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);

		return(parent::edit_dialaction($this->_info['userfeatures']['id']));
	}

	function edit_voicemail()
	{
		$rs = false;

		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);
		else if(($voicemailid = dwho_ulongint($this->get_result_var('userfeatures','voicemailid'))) === 0)
			return($this->_add_voicemail($this->_info['userfeatures']['id']));
		else if(is_object($this->_voicemail) === false
		|| $this->_voicemail->get($voicemailid) === false
		|| ($rs = $this->get_result('voicemail')) === null
		|| $rs === false)
			return($rs);
		return(($this->_return['voicemail'] = $this->_voicemail->edit(false)));
	}

	function edit_phonefunckey()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);
		else if(dwho_issa('phonefunckey',$this->_info) === false)
			return($this->add_phonefunckey($this->_info['userfeatures']['id']));

		$this->_return['phonefunckey'] = $this->delete_phonefunckey();
		$this->_func_key_manager->delete_fk_in_template($this->_info['userfeatures']['func_key_private_template_id']);

		return($this->add_phonefunckey($this->_info['userfeatures']['id']));
	}

	function edit_schedule_path()
	{
		$this->_schedule_path->delete_where(array('path' => 'user', 'pathid' => intval($this->_info['userfeatures']['id'])));

		return $this->_schedule_path->schedule_add(
			$this->get_result('schedule_id'),
			'user',
			$this->_info['userfeatures']['id']);
	}

	function edit_exten($user_id)
	{
		if(($rs = $this->_set_exten('edit',$user_id)) !== true)
			return($rs);

		return parent::edit_exten('exten');
	}

	function edit_device_config()
	{
		if(dwho_issa('linefeatures',$this->_info) === false
		|| ($rs = $this->get_result('linefeatures')) === null
		|| $rs === false)
			return(false);

		$vm_number = (string) $this->get_result_var('voicemail', 'mailbox');

		$existing_device = $this->_info['linefeatures'][0];
		$new_device = $rs[0];

		// Device change, update it.
		if ($existing_device['device'] !== $new_device['device'])
		{
			// Delete the old device associated.
			if (empty($existing_device['device']) === false)
			{
				if (($device = $this->_device->get($existing_device['device'],null,true)) === false)
				{
					dwho_report::push('error', 'device doesn\'t exist: '.$existing_device['device']);
					return(false);
				}
				elseif ($this->_device->remove_line($new_device['id']) === false)
				{
					dwho_report::push('error', 'Can\'t remove line device');
					return(false);
				}
				elseif ($existing_device['protocol'] === XIVO_SRE_IPBX_AST_PROTO_SCCP
				&& $this->_sccpdevice->init_device($device['device']['mac'],'') === false)
				{
					dwho_report::push('error', 'Can\'t initialize SCCP device');
					return(false);
				}
			}
			// Add the new device associated.
			if (empty($new_device['device']) === false)
			{
				if (($device = $this->_device->get($new_device['device'])) === false)
				{
					dwho_report::push('error', 'device doesn\'t exist: '.$new_device['device']);
					return(false);
				}
				elseif($this->_provddevice->update_config_from_line($device['device']['id'],$new_device['id']) === false)
				{
					dwho_report::push('error', 'Can\'t update config device');
					return(false);
				}
				elseif ($new_device['protocol'] === XIVO_SRE_IPBX_AST_PROTO_SCCP
				&& $this->_sccpdevice->init_device($device['device']['mac'],$new_device['number'],$vm_number) === false)
				{
					dwho_report::push('error', 'Can\'t initialize SCCP device');
					return(false);
				}
			}
		}
		// Device has not changed, just update.
		elseif (empty($new_device['device']) === false
		&& empty($existing_device['device']) === false)
		{
			if (($device = $this->_device->get($existing_device['device'])) === false)
			{
				dwho_report::push('error', 'device doesn\'t exist: '.$existing_device['device']);
				return(false);
			}
			elseif($this->_provdconfig->rebuild_device_config($device['device']['id']) === false)
			{
				dwho_report::push('error', 'Can\'t rebuild config device');
				return(false);
			}
			elseif ($new_device['protocol'] === XIVO_SRE_IPBX_AST_PROTO_SCCP
			&& $this->_sccpdevice->init_device($device['device']['mac'],$new_device['number'],$vm_number) === false)
			{
				dwho_report::push('error', 'Can\'t initialize SCCP device');
				return(false);
			}
		}

		return(true);
	}

	function edit_groupmember($user_id)
	{
		return($this->_edit_member_type('group', $user_id));
	}

	function edit_queuemember($user_id)
	{
		return($this->_edit_member_type('queue', $user_id));
	}

	function _edit_member_type($type, $user_id)
	{
		if(($type !== 'group' && $type !== 'queue') === true
		|| dwho_issa('linefeatures',$this->_info) === false)
			return(false);

		$key = $type.'member';
		$keyid = $type.'featuresid';

		$old_qm =  array();
		$new_qm = dwho_array_set_value_to_key($this->get_result($key), $keyid);

		if(dwho_issa($key,$this->_info) === true)
		{
			$old_qm = dwho_array_set_value_to_key($this->_info[$key], $keyid);

			$to_delete = array_diff_key($old_qm, $new_qm);
			$to_edit = array_intersect_key($new_qm, $old_qm);

			foreach ($to_delete as $element)
			{
				$where = array('usertype' => $element['usertype'],
							'userid' => $element['userid'],
							'queue_name' => $element['queue_name']);
				if($this->_queuemember->delete_where($where) === false)
					return(false);

				$this->_return[$key][] = $element;
			}

			foreach ($to_edit as $element)
			{
				$old_element = $old_qm[$element[$keyid]];

				$element['userid'] = $old_element['userid'];
				$element['position'] = $old_element['position'];
				$element['commented'] = $old_element['commented'];

				unset($element['groupfeaturesid'],$element['queuefeaturesid']);

				$id = array('queue_name' => $old_element['queue_name'],
							'interface' => $old_element['interface']);
				if($this->_queuemember->edit($id, $element) === false)
					return(false);

				$this->_return[$key][] = $element;
			}
		}

		$to_add = array_diff_key($new_qm, $old_qm);

		foreach ($to_add as $element)
		{
			$element['userid'] = $user_id;

			unset($element['groupfeaturesid'],$element['queuefeaturesid']);

			if($this->_queuemember->add($element) === false)
				return(false);

			$this->_return[$key][] = $element;
		}

		if(isset($this->_return[$key][0]) === false)
			$this->_return[$key] = false;

		return(true);
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
				case 'userfeatures':
					if(isset($this->_origin['userfeatures']) === false
					|| $this->_origin['userfeatures'] === false)
						$this->delete_userfeatures();
					else
						$this->_userfeatures->edit_origin($this->_origin['userfeatures']);
					break;
				case 'voicemail':
					$this->_voicemail->reverse_edit();
					break;
				case 'rightcall':
					$this->_rightcall->reverse_edit($this->_status);
					break;
				case 'dialaction':
					$this->_dialaction->reverse_edit($this->_status);
					break;
				case 'phonefunckey':
					$this->delete_phonefunckey();

					if(isset($this->_origin['phonefunckey']) === true
					&& $this->_origin['phonefunckey'] !== false)
						$this->_phonefunckey->add_origin_list($this->_origin['phonefunckey']);
					break;
				case 'exten':
					if(isset($this->_origin['exten']) === false
					|| $this->_origin['exten'] === false)
						$this->delete_exten('exten');
					else
						$this->_extensions->edit_origin($this->_origin['exten']);
					break;
				case 'groupmember':
				case 'queuemember':
					if($key === 'groupmember')
						$this->delete_groupmember();
					else
						$this->delete_queuemember();

					if(isset($this->_origin[$key]) === true
					&& $this->_origin[$key] !== false)
						$this->_queuemember->add_origin_list($this->_origin[$key]);
					break;
			}
		}

		$this->_line->_status = 'edit';
		$this->_line->_reverse_edit();

		return(true);
	}

	function delete()
	{
		$r = true;

		$this->_status = 'delete';
		$this->_return = array();

		if ($this->has_existing_line())
		{
			if(isset($this->_info['userfeatures']['voicemailid'])
			&& (int) $this->_info['userfeatures']['voicemailid'] !== 0)
			{
				$this->_set_error('voicemail', dwho_i18n::babelfish('user_no_remove_lines_when_voicemail'));
				dwho_report::push('error', dwho_i18n::babelfish('user_no_remove_lines_when_voicemail'));
				return(false);
			}
		}

		if($this->_delete_func_key_v2() === false
		|| $this->delete_userfeatures() === false
		|| $this->delete_device_config() === false
		|| $this->delete_linefeatures() === false
		|| $this->delete_exten('exten') === false
		|| $this->delete_rightcall() === false
		|| $this->delete_dialaction() === false
		|| $this->delete_phonefunckey() === false
		|| $this->delete_callfiltermember() === false
		|| $this->delete_groupmember() === false
		|| $this->delete_queuemember() === false
		|| $this->delete_schedule_path() === false)
		{
			$r = false;
			$this->_reverse_delete();
		}
		else
			$this->unlinks();

		$this->_status = '';

		return($r);
	}

	private function _delete_func_key_v2()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);

		$this->_func_key_manager->delete_dst_for_user($this->_info['userfeatures']['id']);

		return(true);
	}

	function delete_userfeatures()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);

		$user_id = (int) $this->_info['userfeatures']['id'];
		$func_key_private_template_id = (int) $this->_info['userfeatures']['func_key_private_template_id'];

		$this->_user_line->delete_where(array('user_id' => $user_id));

		$r = $this->_userfeatures->delete($user_id);

		if($this->_status === 'delete')
			$this->_return['userfeatures'] = $r;

		if ($this->_func_key_manager->delete_template($func_key_private_template_id) === false)
			return(false);

		return($r);
	}

	function delete_linefeatures()
	{
		if(dwho_issa('linefeatures',$this->_info) === false)
			return(null);

		$r = true;
		foreach($this->_info['linefeatures'] as $line)
		{
			if($this->_line->get($line['id']) === false
			|| $this->_line->delete() === false)
			{
				$r = false;
				$identity = $this->_info['userfeatures']['identity'];
				dwho_report::push('error','Can\'t remove line (id #'.$line['id'].') for user "'.$identity.'"', 'lines');
			}
		}

		if($this->_status === 'delete')
			$this->_return['linefeatures'] = $r;

		return($r);
	}

	function delete_device_config()
	{
		if(dwho_issa('linefeatures',$this->_info) === false)
			return(null);
		// remove device if associated
		elseif(($device = $this->_device->get($this->_info['linefeatures'][0]['device'],null,true)) === false)
			return(true);
		elseif($this->_device->remove_line($this->_info['linefeatures'][0]['id']) === false)
			return(false);
		elseif ($this->_info['linefeatures'][0]['protocol'] === XIVO_SRE_IPBX_AST_PROTO_SCCP
		&& $this->_sccpdevice->init_device($device['device']['mac'],'') === false)
			return(false);

		return(true);
	}

	function delete_phonefunckey()
	{
		if(dwho_issa('phonefunckey',$this->_info) === false)
			return(null);
		else if(dwho_issa('userfeatures',$this->_info) === false
		|| is_object($this->_phonefunckey) === false)
			return(false);

		$where = array();
		$where['iduserfeatures'] = $this->_info['userfeatures']['id'];

		$r = $this->_phonefunckey->delete_where($where);

		if($this->_status === 'delete')
			$this->_return['phonefunckey'] = $r;

		return($r);
	}

	function delete_callfiltermember()
	{
		if(($callfiltermember = &$this->_service->get_module('callfiltermember')) === false)
			return(true);

		return($callfiltermember->delete_member($this->_info['userfeatures']['id']));
	}

	function delete_schedule_path()
	{
		return $this->_schedule_path->delete_where(array('path' => 'user', 'pathid' =>	$this->_info['userfeatures']['id']));
	}

	function delete_groupmember()
	{
		return($this->_delete_member_type('group'));
	}

	function delete_queuemember()
	{
		return($this->_delete_member_type('queue'));
	}

	function _delete_member_type($type)
	{
		if(($type !== 'group' && $type !== 'queue') === true)
			return(false);

		$key = $type.'member';

		if(dwho_issa($key,$this->_info) === false)
			return(null);
		else if(dwho_issa('userfeatures',$this->_info) === false
		|| is_object($this->_queuemember) === false)
			return(false);

		$where = array();
		$where['usertype'] = 'user';
		$where['category'] = $type;
		$where['userid'] = $this->_info['userfeatures']['id'];

		$r = $this->_queuemember->delete_where($where);

		if($this->_status === 'delete')
			$this->_return[$key] = $r;

		return($r);
	}

	function _reverse_delete()
	{
		if($this->get_errnb() > 0
		|| $this->_status !== 'delete'
		|| is_array($this->_return) === false
		|| empty($this->_return) === true)
			return(false);

		foreach($this->_return as $key => $val)
		{
			if($val === false)
				continue;

			switch($key)
			{
				case 'userfeatures':
					if(isset($this->_origin['userfeatures']) === true
					&& $this->_origin['userfeatures'] !== false)
						$this->_userfeatures->add_origin($this->_origin['userfeatures']);
					break;
				case 'rightcall':
					$this->_rightcall->add_origin_list();
					break;
				case 'dialaction':
					$this->_dialaction->add_origin_list();
					break;
				case 'groupmember':
				case 'queuemember':
					if(isset($this->_origin[$key]) === true
					&& $this->_origin[$key] !== false)
						$this->_queuemember->add_origin_list($this->_origin[$key]);
					break;
				case 'phonefunckey':
					if(isset($this->_origin['phonefunckey']) === true
					&& $this->_origin['phonefunckey'] !== false)
						$this->_phonefunckey->add_origin_list($this->_origin['phonefunckey']);
					break;
				case 'exten':
					if(isset($this->_origin[$key]) === true
					&& $this->_origin[$key] !== false)
						$this->_extensions->add_origin($this->_origin[$key]);
					break;
			}
		}

		return(true);
	}

	function unlinks()
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);

		$this->_dialaction->unlinked('user',$this->_info['userfeatures']['id']);

		return(true);
	}

	function enable()
	{
		$r = true;

		$this->_status = 'enable';
		$this->_return = array();

		if($this->enable_userfeatures() === false)
		{
			$r = false;
			$this->_reverse_enable_disable();
		}

		$this->_status = '';

		return($r);
	}

	function disable()
	{
		$r = true;

		$this->_status = 'disable';
		$this->_return = array();

		if($this->disable_userfeatures() === false)
		{
			$r = false;
			$this->_reverse_enable_disable();
		}

		$this->_status = '';

		return(true);
	}

	function enable_userfeatures()
	{
		return($this->_enable_disable_userfeatures(false));
	}

	function disable_userfeatures()
	{
		return($this->_enable_disable_userfeatures(true));
	}

	function _enable_disable_userfeatures($disable=false)
	{
		if(dwho_issa('userfeatures',$this->_info) === false)
			return(false);
		else if((bool) $disable === false)
			$r = $this->_userfeatures->enable($this->_info['userfeatures']['id']);
		else
			$r = $this->_userfeatures->disable($this->_info['userfeatures']['id']);

		if($this->_status === 'enable' || $this->_status === 'disable')
			$this->_return['userfeatures'] = $r;

		return($r);
	}

	function _reverse_enable_disable()
	{
		if(($this->_status !== 'enable' && $this->_status !== 'disable') === true
		|| is_array($this->_return) === false
		|| empty($this->_return) === true)
			return(false);

		$disable = $this->_status === 'enable';

		$this->_status = '';

		foreach($this->_return as $key => $val)
		{
			if($val === false)
				continue;

			switch($key)
			{
				case 'userfeatures':
					$this->_enable_disable_userfeatures($disable);
					break;
			}
		}

		return(true);
	}

	function _prepare_import($arr, $partial=false)
	{
		if (isset($arr['userfeatures']['enableclient']) === true
		&& isset($arr['userfeatures']['profileclient']) === true
		&& (dwho_has_len($arr['userfeatures']['profileclient']) === true))
		{
			$profile_name = $arr['userfeatures']['profileclient'];
			if (($profile_id = $this->_cti_profile->get_cti_profile_id_with_name($profile_name)) === false)
				return('Wrong CTI profile name');
			$arr['userfeatures']['cti_profile_id'] = $profile_id;
		}

		if(dwho_issa('voicemail',$arr) === true
		&& empty($arr['voicemail']) === false
		&& dwho_has_len($arr['voicemail'],'mailbox') === true) {
			$arr['userfeatures']['enablevoicemail'] = true;
			$arr['userfeatures']['voicemailtype'] = 'asterisk';
		} else {
			unset($arr['voicemail']);
		}

		if (isset($arr['userfeatures']['entityid']) === false)
			$arr['userfeatures']['entityid'] = 1;

		if (isset($arr['userfeatures']['enablexfer']) === false
		|| dwho_has_len($arr['userfeatures']['enablexfer']) === false)
			$arr['userfeatures']['enablexfer'] = 1;

		if(dwho_has_len($arr['userfeatures'],'firstname') === true)
			$arr['userfeatures']['callerid'] = $arr['userfeatures']['firstname'].' ';

		if(dwho_has_len($arr['userfeatures'],'lastname') === true)
			$arr['userfeatures']['callerid'] .= $arr['userfeatures']['lastname'];

		if(dwho_has_len($arr['userfeatures'],'agentnumber') === true
		&& is_object($this->_agentfeatures) === true
		&& ($agentid = $this->_agentfeatures->get_primary(
					array('number' => $arr['userfeatures']['agentnumber']))) !== false) {
			$arr['userfeatures']['agentid'] = $agentid;
		}

		unset($arr['userfeatures']['agentnumber']);

		if(dwho_issa('linefeatures',$arr) === true) {
			if (!dwho_issa('id', $arr['linefeatures'])) {
				$arr['linefeatures']['id'] = array();
				array_push($arr['linefeatures']['id'], 0);
			}
			array_push($arr['linefeatures']['id'], 0);
		}

		return($arr);
	}

	private function _report_error_from_import_csv(&$r, $generic_error, $import_step, $line_number, $line_data)
	{
		$generic_error = is_array($generic_error) ? implode("\n", $generic_error) : $generic_error;
		$error_msg = $generic_error. " - error during $import_step - line #$line_number\n";

		error_log(print_r($generic_error, true)."\n".print_r($line_data, true) . ' userimport - line #'.$line_number);
		array_push($r['total']['error_return'], $error_msg);
		dwho_report::push('error', $generic_error);
	}

	function import_from_array($data,$save=true)
	{
		$save = (bool) $save;

		if(is_array($data) === false
		|| ($nb = count($data)) === 0)
			return(false);

		$appincall = &$this->_service->get_application('incall');

		$incall = array();
		$incall['dialaction'] = array();

		$r = array();
		$r['lines'] = array();
		$r['total'] = array();
		$r['total']['success'] = 0;
		$r['total']['error'] = 0;
		$r['total']['error_return'] = array();
		$r['total']['count'] = $nb;

		for($i = 0;$i < $nb;$i++)
		{
			$ref = &$data[$i];
			$line_number = $i+1;

			if (dwho_issa('userfeatures', $ref) === false)
			{
				$this->_report_error_from_import_csv($r, $this->get_error(), 'Missing userfeatures', $line_number, $ref);
				continue;
			}

			if (($arr = $this->_prepare_import($ref)) === false
			|| is_array($arr) === false)
			{
				$this->_report_error_from_import_csv($r, $arr, $arr, $line_number, $ref);
				continue;
			}

			if($this->set_add($arr) === false)
			{
				$r['lines'][$line_number] = 'error_import';
				$r['total']['error']++;
				$this->_report_error_from_import_csv($r, $this->get_error(), 'create user', $line_number, $ref);
				continue;
			}
			else if(dwho_issa('incall',$arr) === false
			|| dwho_has_len($arr['incall'],'exten') === false)
			{
				if($save === true && $this->add() === false)
				{
					$r['lines'][$line_number] = 'error_user';
					$r['total']['error']++;

					$this->_reverse_add();
					$this->_report_error_from_import_csv($r, $this->get_error(), 'create user', $line_number, $ref);
				}
				else
				{
					$r['lines'][$line_number] = 'success';
					$r['total']['success']++;
				}
				continue;
			}

			$incall['incall'] = $arr['incall'];
			$incall['incall']['context'] = $arr['incall']['context'];

			$incall['dialaction']['answer'] = array();
			$incall['dialaction']['answer']['actiontype'] = 'user';
			$incall['dialaction']['answer']['actionarg1'] = 1;

			if(dwho_has_len($arr['incall'],'ringseconds') === true)
				$incall['dialaction']['answer']['actionarg2'] = $arr['incall']['ringseconds'];
			else
				$incall['dialaction']['answer']['actionarg2'] = '';

			unset($incall['incall']['ringseconds']);
			if($appincall->set_add($incall) === false)
			{
				$r['lines'][$line_number] = 'error_incall';
				$r['total']['error']++;
				$this->_report_error_from_import_csv($r, $appincall->get_error(), 'validate incall', $line_number, $ref);
				continue;
			}
			else if($save === false)
			{
				$r['lines'][$line_number] = 'success';
				$r['total']['success']++;
				continue;
			}
			else if($this->add() === false)
			{
				$r['lines'][$line_number] = 'error';
				$r['total']['error']++;

				$this->_reverse_add();
				$this->_report_error_from_import_csv($r, $this->get_error(), 'create user', $line_number, $ref);
				continue;
			}

			$incall['dialaction']['answer']['actionarg1'] = $this->get_return('userfeatures');

			if($appincall->set_add($incall) === false
			|| $appincall->add() === false)
			{
				$r['lines'][$line_number] = 'error';
				$r['total']['error']++;

				$appincall->_reverse_add();
				$this->_report_error_from_import_csv($r, $appincall->get_error(), 'create incall', $line_number, $ref);
			}
			else
			{
				$r['lines'][$line_number] = 'success';
				$r['total']['success']++;
			}
		}

		return($r);
	}

	function import_csv($save=true)
	{
		$save = (bool) $save;

		if(dwho::load_class('dwho::file::csv') === false
		|| ($http_response = dwho_http::factory('response')) === false
		|| ($fileinfo = $http_response->upload_file('import',
							    $this->_conf['import']['file'])) === false)
			return(false);

		if (($data = $this->_get_data_from_csv_file($fileinfo['tmp_name'], $this->_conf['import']['field'])) === false)
			return(false);

		$r = $this->import_from_array($data,$save);

		if($save === true)
			dwho_file::rm($fileinfo['tmp_name']);

		return($r);
	}

	function import_from_csv()
	{
		$_QRY = &dwho_gct::get('dwho_query');

		if(dwho::load_class('dwho::file::csv') === false
		|| ($data_input = $_QRY->get_input()) === false)
		{
			print("can not get data (maybe missing join data in your POST ?)\n");
			return(false);
		}

		$tmpfname = tempnam('/tmp' , 'import_');
		file_put_contents($tmpfname, $data_input);

		if (($data = $this->_get_data_from_csv_file($tmpfname, $this->_conf['import']['field'])) === false)
			return(false);

		$r = $this->import_from_array($data);

		if ( $r['total']['error'] > 0)
		{
			$error_return = $r['total']['error_return'];
			foreach($error_return as $err)
				print $err."\n";
			dwho_file::rm($tmpfname);
			return(false);
		}

		dwho_file::rm($tmpfname);

		return(true);
	}

	private function _get_data_from_csv_file($csv_file_path, $field_map)
	{
		$filecsv = new dwho_file_csv();

		if(($data = $filecsv->parse($csv_file_path,
					    0,
					    '|',
					    null,
					    true,
					    $field_map)) === false
		|| empty($data) === true)
		{
			print("can not parse csv file (maybe fields are not separated by | ?)\n");
			dwho_file::rm($tmpfname);
			return(false);
		}
		return($data);
	}

	function add_from_json()
	{
		if(($data = $this->_get_data_from_json()) === false
		|| ($arr = $this->_prepare_import($data)) === false
		|| $this->set_add($arr) === false
		|| $this->add() === false)
			return(false);

		return($this->_return['userfeatures']);
	}

	function array_merge_recursive($arr1, $arr2)
	{
		foreach($arr2 as $k => $v)
		{
			if(array_key_exists($k, $arr1) && is_array($arr1[$k]) && is_array($v))
				$arr1[$k] = $this->array_merge_recursive($arr1[$k], $arr2[$k]);
			else
				$arr1[$k] = $v;
		}

		return $arr1;
	}

	function edit_from_json($mergewith=null)
	{
		if(($data = $this->_get_data_from_json()) === false
		|| ($arr = $this->_prepare_import($data, !is_null($mergewith))) === false)
			return(false);

		if(!is_null($mergewith))
		{
			unset($mergewith['linefeatures']);
			$arr = $this->array_merge_recursive($mergewith,$arr);
			if(is_array($arr['rightcall'])) {
			    $rightcall = array();
			    $rightcall[0] = strval($arr['rightcall'][0]['rightcallid']);
			    $arr['rightcall'] = $rightcall;
			}
		}

		if($this->set_edit($arr) === false
		|| $this->edit() === false)
			return(false);

		return(true);
	}

	function gen_password($len, $lower=false)
	{
		$pwd = '';
		for($i = 0; $i < $len; $i++)
		{
			$r = mt_rand(0, 35);
			$pwd .= $r < 10?$r:chr($r-10+65);
		}

		if($lower)
			$pwd = strtolower($pwd);

		return $pwd;
	}

	public function has_existing_line()
	{
		if(isset($this->_info['linefeatures'])
		&& $this->_info['linefeatures'] !== false
		&& count($this->_info['linefeatures']) > 0)
			return(true);
		return(false);
	}
}

?>
