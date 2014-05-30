<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
class FacebookFQL extends CI_Model {
    function __construct() {
        parent::__construct();
        $this->load->library('Facebook');
    }
    
    /**
     * @return an array containing the user's list of friends where each entry is an array containing
     * pic_square, uid, name
     */
    public function getFriends() {
        $fql = "SELECT pic_square, uid, name FROM user WHERE uid IN (SELECT uid2 FROM friend WHERE uid1 = me())";
        $ret = $this->facebook->api(array(
                                        'method' => 'fql.query',
                                        'query' => $fql));
        return $ret;
    }
    
    /**
     * @return an array containing the user's information like firstname and lastname
     */
    public function getUserInfo($uid) {
        $fql = "SELECT first_name, last_name FROM user WHERE uid = $uid";
        return $this->facebook->api(array(
                                        'method' => 'fql.query',
                                        'query' => $fql));
    }
}
?>