<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
/**
 * This controller class will handle the rpc requests for the desktop application
 */
class Xmlrpc extends MY_Controller {    
    public function __construct() {
        parent::__construct();
        $this->load->library('xmlrpc');
        $this->load->model('RpcSQL');
    }
    
    /**
     * @return a json array containing a boolean value
     *      true means the desktop app is up and running
     */
    public function setUid($uid) {
        $request = array($uid);
        $this->xmlrpc->request($request);
        
		$this->xmlrpc->method('setUid');
        
        $requestRPCString = $this->xmlrpc->get_request();
        
        $requestId = $this->RpcSQL->addRequest($_SERVER["REMOTE_ADDR"], $requestRPCString);
        
        echo json_encode(array("request_id" => $requestId));
    }
    
    
    /**
     * Returns a JSON array containing key "request_processed" with value 1 if
     * the request is not in the database anymore - meaning the desktop client
     * has processed it. Returns 0 otherwise.
     */
    public function requestProcessed($uid) {
        if ($this->RpcSQL->processedRequest($uid, $_SERVER["REMOTE_ADDR"])) {
            echo json_encode(array("request_processed" => 1));
        } else {
            echo json_encode(array("request_processed" => 0));
        }
    }
}
