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

require_once(dwho_file::joinpath(DWHO_PATH_ROOT,'date.inc'));

#Hardcode 20000 as absolute limit number of cel records the call logs can handle
#Helps not busting the 128M PHP memory limit (and thus not getting blank page)
define("CEL_FETCH_LIMIT",20000);

class stats_ressource_cel
{

   public      $_type      = '';
   public      $_field     = array('duration_in'       => array('type' => 'time'),
                                   'duration_out'      => array('type' => 'time'),
                                   'duration_intern'   => array('type' => 'time'),
                                   'duration_total'    => array('type' => 'time'),
                                   'nb_in'             => array('type' => 'int'),
                                   'nb_out'            => array('type' => 'int'),
                                   'nb_intern'         => array('type' => 'int'),
                                   'nb_total'          => array('type' => 'int'),
                                   'max_concurrent_calls_in'       => array('type' => 'int'),
                                   'max_concurrent_calls_out'      => array('type' => 'int'),
                                   'max_concurrent_calls_intern'   => array('type' => 'int'),
                                   'max_concurrent_calls_total'    => array('type' => 'int'));
   private     $_result    = false;
   private     $_cel       = false;
   private     $_trunk     = false;
   private     $_ipbx      = false;
   private     $_total     = 0;

   public function __construct()
   {
       $this->_type = 'cel';

       if(($_SRE = &dwho_gct::get('xivo_service')) === false
       || ($this->_ipbx = &$_SRE->get('ipbx')) === false
       || ($this->_cel = &$this->_ipbx->get_module('cel')) === false
       || ($this->_trunk = &$this->_ipbx->get_application('trunk')) === false)
           die('Can\'t load object for xivo_statistics_cel');
   }

   private function _build_trunkname_from_identity($identity)
   {
       if (preg_match('/([\w]+) \(([\w]+)\)/i',$identity,$out) === 0
       || empty($out) === true
       || isset($out[1]) === false
       || isset($out[2]) === false)
           return(false);

       return(($out[2].'/'.$out[1]));
   }

   public function get_cnt()
   {
       return($this->_total);
   }

   public function get_list_call($arr=null, $order=null, $limit=null)
   {
       if (is_null($arr) === true)
       {
           $arr = array();
           $arr['dbeg'] = date('Y-m-d').'';
       }

       if ($this->_trunk === false
       || ($list = $this->_cel->search($arr, $order, $limit)) === false)
           return(false);

       $rs = array();
       $nb = count($list);

       if ($nb === CEL_FETCH_LIMIT)
       {
           dwho_report::push('error', dwho_i18n::babelfish('call_log_reached_cel_fetch_limit'));
       }

       for($i=0;$i<$nb;$i++)
       {
           $ref = &$list[$i];
           $lid = $ref['linkedid'];
           if (isset($rs[$lid]) === false)
               $rs[$lid] = array();
           array_push($rs[$lid], $ref);
       }
       return($rs);
   }

   public function init_result_by_list($arr)
   {
       while($arr)
       {
           $data = array_shift($arr);
           $this->_init_result($data['key']);
       }
   }

   private function _init_result($key)
   {
       $res = &$this->_result;

       if (isset($res[$key]) === false)
       {
           $data_process = &$res[$key];
           $data_process = array();

           foreach ($this->_field as $k => $field)
           {
               $data_process[$k] = array();
               $data_process[$k]['type'] = $field['type'];
               $data_process[$k]['result'] = 0;
           }
       }
   }

   public function get_calls_records($search=null, $order=null, $limit=null)
   {
       return($this->_build_data($search, $order, $limit));
   }

   private function _build_search($pattern, $type)
   {
       switch($type)
       {
           case 'begin':
               $like_format = '/^%s/i';
               break;
           case 'contain':
               $like_format = '/%s/i';
               break;
           case 'end':
               $like_format = '/%s$/i';
               break;
           default:
               $like_format = '/^%s$/i';
       }

       return(sprintf($like_format,$pattern));
   }

   private function _build_data($search=null, $order=null, $limit=null)
   {
       $rs = array();

       if (($listcall = $this->get_list_call($search, $order, CEL_FETCH_LIMIT)) === false)
       {
           return(false);
       }

       $total = count($listcall);

       if ($total === 0)
           return(null);

       if(dwho_has_len($search,'src') === true && isset($search['srcformat']) === true)
           $search_src = $this->_build_search($search['src'], $search['srcformat']);

       if(dwho_has_len($search,'dst') === true && isset($search['dstformat']) === true)
           $search_dst = $this->_build_search($search['dst'], $search['dstformat']);

       if(dwho_has_len($search,'channel') === true && isset($search['channel']) === true)
           $search_channel = $this->_build_search($search['channel'], 'begin');

       foreach ($listcall as $linkedid => $ticketcall)
       {
           $token = $tmp = array();
           $nb = count($ticketcall);
           for($i=0; $i<$nb; $i++)
           {
               $call = &$ticketcall[$i];

               $eventtime = (int) strtotime($call['eventtime']);
               $uniqueid = $call['uniqueid'];
               $eventtype = $call['eventtype'];
               $channame = $call['channame'];
               $trunkname = substr($channame,0,-strlen(strrchr($channame,'-')));

               if (isset($token[$uniqueid]) === false)
                   $token[$uniqueid] = array();

               $ref = &$token[$uniqueid];

               if (isset($ref['event']) === false)
                   $ref['event'] = array();

               $event = &$ref['event'];

               if (isset($event[$eventtype]) === false)
                   $event[$eventtype] = 0;

               $event_cnt = &$event[$eventtype];
               $event_cnt++;

               if (isset($tmp[$uniqueid]) === false)
                   $tmp[$uniqueid] = array();

               $cc = &$tmp[$uniqueid];

               if (!isset($ref['duration']))
                   $ref['duration'] = 0;

               switch ($eventtype)
               {
                   case 'CHAN_START':
                       switch ($event_cnt)
                       {
                           case 1:
                               $ref['uniqueid'] = $uniqueid;
                               $ref['channame'] = $channame;
                               $ref['dstart'] = date('Y-m-d H:i:s',$eventtime);
                               $ref['start'] = $eventtime;
                               $ref['trunk'] = $trunkname;
                               if (dwho_has_len($call['cid_name']) && dwho_has_len($call['cid_num']))
                                   $ref['from'] = $call['cid_name'].' ('.$call['cid_num'].')';
                               if (preg_match('/^'.XIVO_SRE_IPBX_AST_REGEXP_DIALPLAN_EXTEN.'$/', $call['exten']))
                                   $ref['to'] = $call['exten'];
                               $ref['context'] = $call['context'];
                               $ref['userdeftype'] = $call['userdeftype'];
                               $ref['answer'] = false;
                               $ref['userfield'] = $call['userfield'];
                               $ref['accountcode'] = $call['accountcode'];
                               $ref['peer'] = $call['peer'];

                               $cc['ringing'] = $eventtime;
                               break;
                           case 2:
                               if (dwho_has_len($call['cid_name']) && dwho_has_len($call['cid_num']))
                                   $ref['to'] = $call['cid_name'].' ('.$call['cid_num'].')';
                               break;
                           default:
                               $ref['dstart'] = date('Y-m-d H:i:s',$eventtime);
                               $ref['start'] = $eventtime;
                               $ref['context'] = $call['context'];
                               break;
                       }
                       break;
                   case 'APP_START':
                       switch ($event_cnt)
                       {
                           case 1:
                               if (!isset($ref['from']) || !dwho_has_len($ref['from']))
                                   $ref['from'] = $call['cid_name'].' ('.$call['cid_num'].')';
                               if (isset($ref['context']) && $ref['context'] == 'default')
                                   $ref['context'] = $call['context'];
                               if (!isset($ref['userfield']) || !dwho_has_len($ref['userfield']))
                                   $ref['userfield'] = $call['userfield'];
                               break;
                           case 2:
                               if (!dwho_has_len($ref['to']))
                                   $ref['to'] = $call['cid_name'].' ('.$call['cid_num'].')';
                               break;
                       }
                       break;
                   case 'ANSWER':
                       if (!isset($ref['to']) || !dwho_has_len($ref['to']))
                           $ref['to'] = $call['cid_name'];
                       $cc['danswer'] = $eventtime;
                       $ref['answer'] = true;
                       break;
                   case 'BRIDGE_START':
                       if (isset($cc['ringing']))
                           $ref['ringing'] = (int) ($eventtime - $cc['ringing']);
                       break;
                   case 'BRIDGE_END':
                       break;
                   case 'HANGUP':
                       if (isset($cc['danswer']))
                           $ref['duration'] = (int) ($eventtime - $cc['danswer']);
                       break;
                   case 'CHAN_END':
                       $ref['end'] = $eventtime;
                       $ref['dend'] = date('Y-m-d H:i:s',$eventtime);
                       break;
                   default:
               }
           }

           if (isset($search_dst))
           {
               $call_from = current($token);
               if (!isset($call_from['to'])
               || !preg_match($search_dst, $call_from['to']))
                   continue;
           }
           if (isset($search_src))
           {
               $call_from = current($token);
               if (!isset($call_from['from'])
               || !preg_match($search_src, $call_from['from']))
                   continue;
           }
           if (isset($search_channel))
           {
               $call_from = current($token);
               $call_to = next($token);
               if (!preg_match($search_channel, $call_from['channame'])
                       && !preg_match($search_channel, $call_to['channame']))
                   continue;
           }

           array_push($rs,$token);
       }

       $this->_total = count($rs);

       if ($this->_total === 0)
           return(null);

       if ($limit === null)
           $limit = array(0, $this->_total);
       elseif(is_numeric($limit))
           $limit = array(0, $limit);

       if (($delta = (int) ($limit[0] + $limit[1])) > $this->_total)
           $delta = $this->_total;

       $result = array();
       for ($i=$limit[0]; $i < $delta; $i++)
       {
           $ref = &$rs[$i];
           array_push($result, $ref);
       }
       return($result);
   }

}

?>
