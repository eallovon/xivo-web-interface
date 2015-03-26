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

require_once(XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'config.inc');

define('XIVO_PATH_OBJECT',XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'object');
define('XIVO_PATH_OBJECTCONF',XIVO_PATH_OBJECT.DIRECTORY_SEPARATOR.'objectconf');

require_once(XIVO_PATH_OBJECTCONF.DIRECTORY_SEPARATOR.'define.inc');

require_once(DWHO_PATH_ROOT.DIRECTORY_SEPARATOR.'dwho.inc');

class xivo extends dwho
{
	public function __construct()
	{
		if(($this->_conf['user'] = dwho_gat::load_get('user',XIVO_PATH_OBJECTCONF)) === false)
			die('ERR: Failed to load user Array');

		parent::__construct();

		$this->_conf['xivo'] = &$this->_conf['dwho'];

		if(($xivo = dwho_gat::load_get('xivo',XIVO_PATH_OBJECTCONF)) !== false)
			$this->_conf['xivo'] = dwho_array_merge_replace_recursive($this->_conf['xivo'],$xivo);

		$this->_init_cfg();
		$this->_set_report();
		$this->_init_datastorage();
		$this->_set_error();
		$this->_set_query();
		$this->_set_language();
		$this->_set_session();
		$this->_set_i18n();
		$this->_set_xivo_object();
		$this->_set_service();
		$this->_set_stats();
		$this->_set_confd();
		$this->_set_slack();
		$this->_set_manager();

		if(dwho_constant('XIVO_WEBI_CONFIGURED',false) === true)
			$this->_set_user();

		if (isset($_SESSION["_USR"]->entity_id)
		&& ($entity_id = $_SESSION["_USR"]->entity_id) > 0
		&& $_SESSION["_USR"]->meta != 'root')
		{
			$_XOBJ = &dwho_gct::get('xivo_object');
			$_SRE = dwho_gct::get('xivo_service');
			$modcontext = &$_SRE->get_module('ipbx', 'context');
			$modentity = &$_XOBJ->get_module('entity');

			if (($entity = $modentity->get($entity_id)))
			{
				define('ENTITY_FILTER_NAME', $entity['name']);

				if (($contexts_filter = $modcontext->get_contexts_name_by_entity_name($entity['name'],null,null,null,true)))
					define('ENTITY_FILTER_CONTEXTS', implode(',', array_keys($contexts_filter)));
				else
					define('ENTITY_FILTER_CONTEXTS', '5ihrWK6nf5QWoxIcDlSloOMar31zu6M0Po0AXIr');
			}

			define('ENTITY_FILTER_ID', $entity_id);
		}

		dwho_gat::set('_CF',$this->_conf);
	}

	protected function _init_cfg()
	{
		if(($xivo_ini = parent::load_init(XIVO_PATH_CONF.DWHO_SEP_DIR.'xivo.ini')) === false)
			die('ERR: Missing file: '.XIVO_PATH_CONF.DWHO_SEP_DIR.'xivo.ini');

		define('XIVO_WEBI_CONFIGURED', (bool) file_exists(XIVO_LIB.'/configured'));

		if(dwho_issa('locale',$xivo_ini) === true)
			$this->_cfg_ini_locale($xivo_ini['locale']);
		else
			$this->_cfg_ini_locale();

		if(dwho_issa('session',$xivo_ini) === true)
			$this->_cfg_ini_session($xivo_ini['session']);
		else
			$this->_cfg_ini_session();

		if(dwho_issa('error',$xivo_ini) === true)
			$this->_cfg_ini_error($xivo_ini['error']);

		if(dwho_issa('report',$xivo_ini) === true)
			$this->_cfg_ini_report($xivo_ini['report']);

		$this->_cfg_ini_set();
		$this->_conf['location'] = array();
		$this->_conf['location']['action'] = XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'action/www';
		$this->_conf['location']['script'] = XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'www';
		$this->_conf['template'] = array();
		$this->_conf['template']['www']['path'] = XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'tpl/www';
		$this->_conf['template']['json']['path'] = XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'tpl/json';
		$this->_conf['template']['ui']['path'] = XIVO_PATH_ROOT.DIRECTORY_SEPARATOR.'tpl/ui';

		foreach($this->_conf['dwho']['ini']['report'] as $type => $rp)
			define('REPORT_'.strtoupper($type),(bool) $rp);

		ini_set('include_path',XIVO_PATH_ROOT.DWHO_SEP_PATH.ini_get('include_path'));

		if(dwho_constant('XIVO_WEBI_CONFIGURED',true) === false)
			return(null);

		if(dwho_issa('service',$xivo_ini) === true)
		{
			if(isset($this->_conf['xivo']['ini']['service']) === true)
				$this->_conf['xivo']['ini']['service'] = array_merge(
						(array) $this->_conf['xivo']['ini']['service'],
						$xivo_ini['service']);
			else
				$this->_conf['xivo']['ini']['service'] = $xivo_ini['service'];
		}

		if(dwho_issa('sysconfd',$xivo_ini) === true)
		{
			if(isset($this->_conf['xivo']['ini']['sysconfd']) === true)
				$this->_conf['xivo']['ini']['sysconfd'] = array_merge(
						(array) $this->_conf['xivo']['ini']['sysconfd'],
						$xivo_ini['sysconfd']);
			else
				$this->_conf['xivo']['ini']['sysconfd'] = $xivo_ini['sysconfd'];
		}

		if(defined('XIVO_WEBI_CONFIGURED') === false)
			define('XIVO_WEBI_CONFIGURED',true);

		return(true);
	}

	protected function _init_datastorage()
	{
		if(($dso_conf = parent::load_init(XIVO_CONF.DWHO_SEP_DIR.'datastorage.conf')) === false)
			die('ERR: Missing datastorage file in XIVO');

		$this->_conf['datastorage'] = $dso_conf;

		if(dwho_issa('general',$dso_conf) === true) {
			if (isset($dso_conf['general']['xivo']) === true)
				$this->_cfg_ini_datastorage($dso_conf['general']['xivo']);
			else
				die('ERR: Missing xivo param in general section in XIVO in datastorage file');
		}
	}

	protected function _set_language()
	{
		$_QRY = dwho_gct::get('dwho_query');
		$hl = $_QRY->get('hl');

		if(dwho_has_len($hl) === false || defined('DWHO_USER_I18N_LANGUAGE') === true)
			return(null);

		$_LANG = dwho_gat::load_get('language',XIVO_PATH_OBJECTCONF);

		if(isset($_LANG[$hl]) === false || (bool) $_LANG[$hl] === false)
			return(false);

		define('DWHO_USER_I18N_LANGUAGE',$hl);

		return(true);
	}

	protected function _set_xivo_object()
	{
		self::load_class('xivo_object',XIVO_PATH_OBJECT,'xivo',false);
		dwho_gct::set(new xivo_object());
	}

	protected function _set_service()
	{
		self::load_class('xivo_service',XIVO_PATH_OBJECT,null,false);
		dwho_gct::set(new xivo_service($this->_conf['xivo']['ini']['service']));
	}

	function _set_stats()
	{
		xivo::load_class('xivo_stats',XIVO_PATH_OBJECT,'stats',false);
		dwho_gct::set(new xivo_stats());
	}

	function _set_confd()
	{
		xivo::load_class('xivo_confd',XIVO_PATH_OBJECT,'confd',false);
		dwho_gct::set(new xivo_confd());
	}

	function _set_slack()
	{
		xivo::load_class('xivo_slack',XIVO_PATH_OBJECT,'slack',false);
		dwho_gct::set(new xivo_slack());
	}

	function _set_manager()
	{
		xivo::load_class('xivo_manager',XIVO_PATH_OBJECT,'manager',false);
		dwho_gct::set(new xivo_manager());
	}

	protected function _set_user()
	{
		self::load_class('xivo_user',XIVO_PATH_OBJECT,null,false);
		new xivo_user();
	}

	public function load_datastorage($dso)
	{
		return($this->_cfg_ini_datastorage($dso));
	}

	public static function load_class($class,$dir='',$file='',$lib=true)
	{
		if(strncmp('xivo_',$class,5) === 0
				&& dwho_has_len($file) === false)
			$file = substr($class,5);

		return(parent::load_class($class,$dir,$file,$lib));
	}
}

?>
