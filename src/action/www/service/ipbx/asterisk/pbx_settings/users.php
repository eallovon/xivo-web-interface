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

dwho::load_class('dwho_prefs');
$prefs = new dwho_prefs('users');

$act     = isset($_QR['act']) === true ? $_QR['act'] : '';
$page    = dwho_uint($prefs->get('page', 1));
$search  = strval($prefs->get('search', ''));
$sort    = $prefs->flipflop('sort', 'fullname');

$appschedule = &$ipbx->get_application('schedule');
$modentity = &$_XOBJ->get_module('entity');

$param = array();
$param['act'] = 'list';

if($search !== '')
	$param['search'] = $search;

switch($act)
{
	case 'add':
	case 'edit':
		$appuser = &$ipbx->get_application('user');
		$appprovdconfig = &$_XOBJ->get_application('provdconfig');
		$order = array('displayname' => SORT_ASC);
		$list_configregistrar = $appprovdconfig->get_config_list('',$order,false,false,false,'registrar');

		$device_api = &$_RAPI->get_ressource('device');
		$list_device_line = $device_api->find_all();

		$_TPL->set_var('list_configregistrar',$list_configregistrar);
		$_TPL->set_var('list_device_line',$list_device_line);
		$_TPL->set_var('import_file',$appuser->get_config_import_file());

		include(dirname(__FILE__).'/users/'.$act.'.php');

		$dhtml = &$_TPL->get_module('dhtml');
		$dhtml->set_js('js/dwho/submenu.js');
		$dhtml->set_js('js/dwho/uri.js');
		$dhtml->set_js('js/dwho/http.js');
		$dhtml->set_js('js/dwho/suggest.js');
		$dhtml->set_js('js/service/ipbx/'.$ipbx->get_name().'/dialaction.js');
		$dhtml->set_js('js/service/ipbx/'.$ipbx->get_name().'/phonefunckey.js');
		$dhtml->set_js('js/service/ipbx/'.$ipbx->get_name().'/users.js');
		$dhtml->set_js('js/service/ipbx/'.$ipbx->get_name().'/users/lines.js');
		$dhtml->add_js('/bloc/service/ipbx/'.$ipbx->get_name().'/pbx_settings/users/phonefunckey/phonefunckey.js.php');
		$dhtml->load_js_multiselect_files();

		// timepicker
		$dhtml->set_css('extra-libs/timepicker/jquery-ui-timepicker-addon.css',true);
		$dhtml->set_js('extra-libs/timepicker/jquery-ui-timepicker-addon.js',true);

		// autocomplete
		$dhtml->set_css('extra-libs/jquery-ui/themes/ui-lightness/jquery.ui.autocomplete.css',true);
		$dhtml->set_js('extra-libs/jquery-ui/ui/jquery.ui.autocomplete.js',true);

		// select2
		$dhtml->set_css('extra-libs/select2/select2-xivo.css', true);
		$dhtml->set_js('extra-libs/select2/select2.min.js', true);

		break;
	case 'delete':
		$param['page'] = $page;

		$appuser = &$ipbx->get_application('user');

		if(isset($_QR['id']) === false || $appuser->get($_QR['id']) === false)
			$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);

		$appuser->delete();

		$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);
		break;
	case 'deletes':
		$param['page'] = $page;

		if(($values = dwho_issa_val('users',$_QR)) === false)
			$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);

		$appuser = &$ipbx->get_application('user');

		$nb = count($values);

		for($i = 0;$i < $nb;$i++)
		{
			if($appuser->get($values[$i]) !== false)
				$appuser->delete();
		}

		$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);
		break;
	case 'enables':
	case 'disables':
		$param['page'] = $page;

		if(($values = dwho_issa_val('users',$_QR)) === false)
			$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);

		$appuser = &$ipbx->get_application('user',null,false);

		$nb = count($values);

		for($i = 0;$i < $nb;$i++)
		{
			if($appuser->get($values[$i]) === false)
				continue;
			else if($act === 'disables')
				$appuser->disable();
			else
				$appuser->enable();
		}

		$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);
		break;
	case 'import':
		$appuser = &$ipbx->get_application('user');

		if(isset($_QR['fm_send']) === true)
		{
			$appuser->import_csv();
			$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);
		}

		$_TPL->set_var('import_file',$appuser->get_config_import_file());
		break;
	case 'list':
	default:
		$act = 'list';
		$prevpage = $page - 1;
		$nbbypage = XIVO_SRE_IPBX_AST_NBBYPAGE;

		$appuser = &$ipbx->get_application('user');

		$order = array();
		if($sort[1] == 'fullname')
		{
			$order['firstname'] = $sort[0];
			$order['lastname']  = $sort[0];
		} else {
			$order[$sort[1]] = $sort[0];
		}

		$limit = array();
		$limit[0] = $prevpage * $nbbypage;
		$limit[1] = $nbbypage;

		if($search !== '')
			$list = $appuser->get_users_search($search,null,$order,$limit);
		else
			$list = $appuser->get_users_list(null,$order,$limit);

		$total = $appuser->get_cnt();

		if($list === false && $total > 0 && $prevpage > 0)
		{
			$param['page'] = $prevpage;
			$_QRY->go($_TPL->url('service/ipbx/pbx_settings/users'),$param);
		}

		$_TPL->set_var('pager',dwho_calc_page($page,$nbbypage,$total));
		$_TPL->set_var('list',$list);
		$_TPL->set_var('search',$search);
		$_TPL->set_var('sort',$sort);
}

$_TPL->set_var('act',$act);
$_TPL->set_var('schedules',$appschedule->get_schedules_list());
$_TPL->set_var('entity_list',$modentity->get_all());

$menu = &$_TPL->get_module('menu');
$menu->set_top('top/user/'.$_USR->get_info('meta'));
$menu->set_left('left/service/ipbx/'.$ipbx->get_name());
$menu->set_toolbar('toolbar/service/ipbx/'.$ipbx->get_name().'/pbx_settings/users');

$_TPL->set_bloc('main','service/ipbx/'.$ipbx->get_name().'/pbx_settings/users/'.$act);
$_TPL->set_struct('service/ipbx/'.$ipbx->get_name());
$_TPL->display('index');

?>
