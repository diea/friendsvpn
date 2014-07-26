<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
/**
 * This controller class does SQL requests for the desktop application and replies with a JSON encoding
 * It is called a REST API.
 */
class Rest extends MY_Controller {    
    public function __construct() {
        parent::__construct();
        $this->load->library('xmlrpc');
        $this->load->model('RpcSQL');
        $this->load->model('usersql');
        $this->load->model('restsql');
    }
    
    public function fetchXmlRpc() {
        echo "test<br>";
        var_dump($_POST);
    }
}
