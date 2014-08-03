<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
class RestSQL extends CI_Model {
    function __construct() {
        parent::__construct();
    }
    
    public function fetchXmlRpc($ipArr) {
        $userSQL = "SELECT MIN(id) as id, req, ipv6 FROM XMLRPC WHERE ";
        for ($i = 0; $i < count($ipArr) - 1; $i++) {
            $userSQL .= "ipv6 = ? OR ";
        }
        $userSQL .= "ipv6 = ?";
        $query = $this->db->query($userSQL, $ipArr);
        if ($query->num_rows() > 0) {
            $res = $query->result_array()[0];
            $delSQL = "DELETE FROM XMLRPC WHERE id = ?";
            $this->db->query($delSQL, array($res["id"]));
            return $res;
        } else {
            return false;
        }
    }
    
    public function insertService($service) {
        $sql = "INSERT INTO Service VALUES(?, ?, ?)";
        $this->db->query($sql, $service);
    }
    
    public function insertDevice($device) {
        $sql = "INSERT INTO Record VALUES(?, ?, ?, ?, ?, ?)";
        $this->db->query($sql, $device);
    }
    
    public function getFriends($uid) {
        // if I have authorized a user for a given service he becomes "friend"
        $sql = "SELECT uid, ipv6, certificate FROM User WHERE uid IN (SELECT User_uid as uid FROM Authorized_user WHERE Record_Service_User_uid = ?)";
        $res = $this->db->query($sql, array($uid));
        
        $res_arr = $res->result_array();
        
        // also get users that shared something to me
        $sql = "SELECT uid, ipv6, certificate FROM User WHERE uid IN (SELECT Record_Service_User_uid as uid FROM Authorized_user WHERE User_uid = ? AND Record_Service_User_uid != ?)";
        $res = $this->db->query($sql, array($uid, $uid));
        
        $new_arr = $res->result_array();
        for ($i = 0; $i < count($new_arr); $i++) {
            $got = false;
            foreach ($res_arr as $oldres) {
                if ($oldres["uid"] == $new_arr[$i]["uid"]) { // already got this friend, ignore
                    $got = true;
                }
            }
            if (!$got) {
                array_push($res_arr, $new_arr[$i]);
            }
        }        
        
        return $res_arr;
    }
    
    public function getLocalIP($uid) {
        $sql = "SELECT ipv6 FROM User WHERE uid = ?";
        $res = $this->db->query($sql, array($uid));
        
        if ($res->num_rows() > 0) {
            return $res->result_array()[0];
        }
        return false;
    }
    
    public function getName($uid) {
        $sql = "SELECT firstname, lastname FROM User WHERE uid = ?";
        $res = $this->db->query($sql, array($uid));
        if ($res->num_rows() > 0) {
            return $res->result_array()[0];
        }
        return false;
    }
    
    public function getUidFromIP($ip) {
        $sql = "SELECT uid FROM User WHERE ipv6 = ?";
        $res = $this->db->query($sql, array($ip));
        if ($res->num_rows() > 0) {
            return $res->result_array()[0];
        }
        return false;
    }
    
    public function getRecordsFor($friendUid, $uid) {
        $sql = "SELECT * FROM Authorized_user WHERE User_uid = ? AND Record_Service_User_uid = ?";
        $res = $this->db->query($sql, array($friendUid, $uid));
        
        if ($res->num_rows() > 0) {
            return $res->result_array();
        }
        return false;
    }
    
    public function pushCert($cert, $uid) {
        $sql = "UPDATE User SET certificate=? WHERE uid = ?";
        $this->db->query($sql, array($cert, $uid));
    }
}
?>