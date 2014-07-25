<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
class Welcome extends MY_Controller {    
    public function __construct() {
        parent::__construct();

        $this->load->library('Facebook');//, $config);
        $this->load->model('UserSQL');
    }

	public function index() {
	    // Try to get the user's id on Facebook
        $userId = $this->facebook->getUser();
 
        // If user is not yet authenticated, the id will be zero
        if($userId == 0) {
            // Generate a login url
            $data['url'] = $this->facebook->getLoginUrl(array('scope'=>'email'));
            $data['needLogin'] = true;
            $this->render('home', $data);
        } else {
            /**
             * Initiate user (set IPv6)
             */
            $data['userId'] = $userId;
            $this->load->model("FacebookFQL");
            $this->FacebookFQL->getFriends();
            /* render home view */
            $this->render('home', $data);
        }
	}
	
	public function getv6() {
        header("Access-Control-Allow-Origin: *"); // allow CORS
	    // initiate user
	    $userId = $this->input->post("userId");
	    $this->UserSQL->initUser($userId);
    	echo json_encode(array("ipv6" => $_SERVER["REMOTE_ADDR"]));
	}
}

/* End of file welcome.php */
/* Location: ./application/controllers/welcome.php */