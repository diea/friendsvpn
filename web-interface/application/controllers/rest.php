<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
/**
 * This controller class does SQL requests for the desktop application and replies with a JSON encoding
 * It is called a REST API.
 */
class Rest extends MY_Controller {  
    private $json_decoded;
      
    public function __construct() {
        parent::__construct();
        $this->load->library('xmlrpc');
        $this->load->model('RpcSQL');
        $this->load->model('usersql');
        $this->load->model('restsql');
        $json = file_get_contents('php://input');
        $this->json_decoded = json_decode($json);
    }
    
    public function fetchXmlRpc() {
        echo json_encode($this->restsql->fetchXmlRpc($this->json_decoded));
    }
    
    public function insertService() {
        $this->restsql->insertService($this->json_decoded);        
    }
    
    public function insertDevice() {
        $this->restsql->insertDevice($this->json_decoded);
    }
    
    public function getFriends() {
        echo json_encode($this->restsql->getFriends($this->json_decoded->uid));
    }
    
    public function getLocalIP() { /* returns the IP for the user stored in the DB */
        echo json_encode($this->restsql->getLocalIP($this->json_decoded->uid));
    }
    
    public function getName() {
        echo json_encode($this->restsql->getName($this->json_decoded->uid));
    }
    
    public function getUidFromIP() {
        echo json_encode($this->restsql->getUidFromIP($this->json_decoded->ip));
    }
    
    public function getRecordsFor() {
        echo json_encode($this->restsql->getRecordsFor($this->json_decoded->friendUid, $this->json_decoded->uid));
    }
    
    public function pushCert() {
        $this->restsql->pushCert($this->json_decoded->cert, $this->json_decoded->uid);
    }
}
