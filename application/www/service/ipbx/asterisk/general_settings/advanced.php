<?php

$appagents = &$ipbx->get_apprealstatic('agents');
$appgeneralagents = &$appagents->get_module('general');

$appgeneralqueue = &$ipbx->get_apprealstatic('generalqueue');

$appmeetme = &$ipbx->get_apprealstatic('meetme');
$appgeneralmeetme = &$appmeetme->get_module('general');

$info = array();
$info['generalagents'] = $appgeneralagents->get_all_by_category();
$info['generalqueue'] = $appgeneralqueue->get_all_by_category();
$info['generalmeetme'] = $appgeneralmeetme->get_all_by_category();

$element = array();
$element['generalagents'] = $appgeneralagents->get_elements();
$element['generalqueue'] = $appgeneralqueue->get_elements();
$element['generalmeetme'] = $appgeneralmeetme->get_elements();

$error = array();
$error['generalagents'] = array();
$error['generalqueue'] = array();
$error['generalmeetme'] = array();

$fm_save = null;

if(isset($_QR['fm_send']) === true)
{
	if(xivo_issa('generalagents',$_QR) === false)
		$_QR['generalagents'] = array();

	if(($rs = $appgeneralagents->set_save_all($_QR['generalagents'])) !== false)
	{
		$info['generalagents'] = $rs['result'];
		$error['generalagents'] = $rs['error'];

		if(isset($rs['error'][0]) === true)
			$fm_save = false;
		else if($fm_save !== false)
			$fm_save = true;
	}

	if(xivo_issa('generalqueue',$_QR) === false)
		$_QR['generalqueue'] = array();

	if(($rs = $appgeneralqueue->set_save_all($_QR['generalqueue'])) !== false)
	{
		$info['generalqueue'] = $rs['result'];
		$error['generalqueue'] = $rs['error'];

		if(isset($rs['error'][0]) === true)
			$fm_save = false;
		else if($fm_save !== false)
			$fm_save = true;
	}

	if(xivo_issa('generalmeetme',$_QR) === true
	&& ($rs = $appgeneralmeetme->set_save_all($_QR['generalmeetme'])) !== false)
	{
		$info['generalmeetme'] = $rs['result'];
		$error['generalmeetme'] = $rs['error'];

		if(isset($rs['error'][0]) === true)
			$fm_save = false;
		else if($fm_save !== false)
			$fm_save = true;
	}
}

$_HTML->assign('fm_save',$fm_save);
$_HTML->assign('error',$error);
$_HTML->assign('generalagents',$info['generalagents']);
$_HTML->assign('generalqueue',$info['generalqueue']);
$_HTML->assign('generalmeetme',$info['generalmeetme']);
$_HTML->assign('element',$element);

$dhtml = &$_HTML->get_module('dhtml');
$dhtml->set_js('js/service/ipbx/'.$ipbx->get_name().'/submenu.js');

$menu = &$_HTML->get_module('menu');
$menu->set_top('top/user/'.$_USR->get_info('meta'));
$menu->set_left('left/service/ipbx/'.$ipbx->get_name());

$_HTML->set_bloc('main','service/ipbx/'.$ipbx->get_name().'/general_settings/advanced');
$_HTML->set_struct('service/ipbx/'.$ipbx->get_name());
$_HTML->display('index');

?>
