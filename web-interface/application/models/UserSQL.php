<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
class UserSQL extends CI_Model {
    function __construct() {
        parent::__construct();
    }
    
    /**
     * Initializes a user of uid $uid; if not in the database the user is created, otherwise his IP is updated
     */
    public function initUser($uid) {
        // get user v6, if it fails return false
        $user_v6 = $_SERVER["REMOTE_ADDR"]; // make sure user will use his IPv6 !
    
        $userSQL = "SELECT uid, firstname, lastname FROM User WHERE uid = ?";
        $query = $this->db->query($userSQL, array($uid));
        if ($query->num_rows() > 0) {
            $sqlUpdateIp = "UPDATE User SET ipv6 =\"".$user_v6."\" WHERE uid = ?";
            $this->db->query($sqlUpdateIp, array($uid));
        } else {
            $this->load->model("FacebookFQL");
            $info = $this->FacebookFQL->getUserInfo($uid);
            
            
            $newUserSQL = "INSERT INTO User VALUES(?, ?, ?, ?, ?)";
            $this->db->query($newUserSQL, array($uid, $info[0]["first_name"], $info[0]["last_name"], $user_v6, ""));
        }
    }
    
    /**
     * Get user v6 from DB
     */
    public function getIpFromDB($uid) {
        $sql = "SELECT ipv6 from User WHERE uid = ?";
        $query = $this->db->query($sql, array($uid));
        
        if ($query->num_rows() > 0) {
            return $query->result_array()[0]["ipv6"];
        }
        return false;
        
    }
    
    /**
     * Will generate a self signed certificate and will return an array
     * containing the certificate X509 at position 0 and private key and position 1
     */
    private function selfSignedCertificate() {
        $dn = array(
            "countryName" => "BE",
            "stateOrProvinceName" => "Liege",
            "localityName" => "Liege",
            "organizationName" => "ULg",
            "organizationalUnitName" => "ULg",
            "commonName" => "facebookApp",
            "emailAddress" => "noreply@noreply.com"
        );
            
        $privkey = openssl_pkey_new();
        
        $csr = openssl_csr_new($dn, $privkey);
        
        $sscert = openssl_csr_sign($csr, null, $privkey, 365); // TODO find a mechanism to renew cert
        
        openssl_x509_export($sscert, $certout);
        openssl_pkey_export($privkey, $pkeyout);
        
        return array($certout, $pkeyout);
    }
    
    /**
     * @return user ip
     * @return false if not existent
     */
    public function getUserIP($uid) {
        $sql = "SELECT ipv6 WHERE uid = ?";
        $query = $this->db->query($sql, array($uid));
        if ($query->num_rows() > 0)
            return $query->result_array();
        return false;
    }
    
    /**
     * Retrieves the hostnames of a given service for user $uid
     */
    public function getHostNames($uid, $service) {
        $sql = "SELECT DISTINCT(hostname) FROM Record WHERE Service_User_uid = ? AND Service_name = ?";
        $query = $this->db->query($sql, array($uid, $service));
        if ($query->num_rows() > 0)
            return $query->result_array();
        return false;
    }
    /**
     * Retrieves $uid's services
     */
    public function getServices($uid) {
        $sql = "SELECT DISTINCT name, trans_prot FROM Service WHERE user_uid = ?";
        $query = $this->db->query($sql, array($uid));
        if ($query->num_rows() > 0)
            return $query->result_array();
        return false;
    }
    /**
     * Retrieves the service's names and port for a given $uid and $hostname and $service which is the service's name
     */
    public function getNamesAndPort($uid, $hostname, $service) {
        $sql = "SELECT name, port, Service_Trans_prot FROM Record WHERE Service_User_uid = ? AND hostname = ? AND Service_name = ?";
        $query = $this->db->query($sql, array($uid, $hostname, $service));
        if ($query->num_rows() > 0)
            return $query->result_array();
        return false;
    }
    /**
     * Returns true if $uid is in the database, false otherwise
     */
    public function friendInDb($uid) {
        $sql = "SELECT uid FROM User WHERE uid = ?";
        $query = $this->db->query($sql, array($uid));
        if ($query->num_rows() > 0)
            return true;
        return false;
    }
    
    /**
     * Returns true if $meUid has authorized $friendUid for $service $hostname $name $port and $transProt
     * False otherwise
     */
    public function friendAuthorized($meUid, $friendUid, $service, $hostname, $name, $port, $transProt) {
        $sql = "SELECT User_uid FROM Authorized_user WHERE User_uid = ? AND Record_hostname = ? AND Record_Service_name = ? AND Record_Service_User_uid = ? AND Record_Service_Trans_prot = ? AND Record_port = ? AND Record_name = ?";
        $query = $this->db->query($sql, array($friendUid, $hostname, $service, $meUid, $transProt, $port, $name));
        if ($query->num_rows() > 0)
            return true;
        return false;
    }
    /**
     * De-authorize a user for a given service
     */
    public function deAuthorizeUser($meUid, $friendUid, $service, $hostname, $name, $port, $transProt) {
        $sql = "DELETE FROM Authorized_user WHERE User_uid = ? AND Record_hostname = ? AND Record_Service_name = ? AND Record_Service_User_uid = ? AND Record_Service_Trans_prot = ? AND Record_port = ? AND Record_name = ?";
        $this->db->query($sql, array($friendUid, $hostname, $service, $meUid, $transProt, $port, $name));
    }
    
    public function authorizeUser($meUid, $friendUid, $service, $hostname, $name, $port, $transProt) {
        $sql = "INSERT INTO Authorized_user VALUES(?, ?, ?, ?, ?, ?, ?)";
        $this->db->query($sql, array($friendUid, $hostname, $service, $meUid, $transProt, $name, $port));
    }

    public function removeRecord($meUid, $friendUid, $service, $hostname, $name, $port, $transProt) {
        $sql = "DELETE FROM Authorized_user WHERE Record_hostname = ? AND Record_Service_name = ? AND Record_Service_User_uid = ? AND Record_Service_Trans_prot = ? AND Record_name = ? AND Record_port = ?";
        $this->db->query($sql, array($hostname, $service, $meUid, $transProt, $name, $port));
        $sql = "DELETE FROM Record WHERE hostname = ? AND Service_name = ? AND Service_User_uid = ? AND Service_Trans_prot = ? AND name = ? AND port = ?";
        $this->db->query($sql, array($hostname, $service, $meUid, $transProt, $name, $port));
        
        if (!$this->getNamesAndPort($meUid, $hostname, $service)) {
            // no more records, we delete the service
            $sql = "DELETE FROM Service WHERE name = ? AND user_uid = ? AND trans_prot = ?";
            $this->db->query($sql, array($service, $meUid, $transProt));
        }
    }
    
    public function removeAllRecords($meUid) {
        $sql = "DELETE FROM Authorized_user WHERE Record_Service_User_uid = ?";
        $this->db->query($sql, array($meUid));
        
        $sql = "DELETE FROM Record WHERE Service_User_uid = ?";
        $this->db->query($sql, array($meUid));
        
        $sql = "DELETE FROM Service WHERE user_uid = ?";
        $this->db->query($sql, array($meUid));
    }
}
?>
























