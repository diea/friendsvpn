<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
class RpcSQL extends CI_Model {
    function __construct() {
        parent::__construct();
    }
    
    public function addRequest($ip, $req) {
        $sql = "INSERT INTO XMLRPC (req, ipv6) VALUES(?,?)";
        $this->db->query($sql, array($req, $ip));
        return $this->db->insert_id();
    }
    
    /**
     * Returns false if XMLRPC request with uid "$uid" and ipv6 "$ipv6"
     * is still in database.
     * True otherwise.
     */
    public function processedRequest($uid, $ipv6) {
        $sql = "SELECT id FROM XMLRPC WHERE id = ? AND ipv6 = ?";
        $resp = $this->db->query($sql, array($uid, $ipv6));
        if ($resp->num_rows() > 0) {
            return false;
        }
        return true;
    }
}
?>