<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');
class Welcome extends MY_Controller {    
    public function __construct() {
        parent::__construct();
        //parse_str($_SERVER['QUERY_STRING'], $_REQUEST); XXX
        //$this->config->load("facebook", TRUE);
        //$config = $this->config->item('facebook');
        $this->load->library('Facebook');//, $config);
        $this->load->model('UserSQL');
    }

	public function index() {
	    // Try to get the user's id on Facebook
        $userId = $this->facebook->getUser();
 
        // If user is not yet authenticated, the id will be zero
        if($userId == 0){
            // Generate a login url
            $data['url'] = $this->facebook->getLoginUrl(array('scope'=>'email'));
            $data['needLogin'] = true;
            $this->render('home', $data);
        } else {
            /**
             * Initiate user (set IPv6)
             */
            $this->UserSQL->initUser($userId);
            $data['userId'] = $userId;
            // Get user's data and print it
            //$user = $this->facebook->api('/me');
            $this->load->model("FacebookFQL");
            $friends = $this->FacebookFQL->getFriends(); // need to exec a first command to init cookies etc.
            //print_r($friends);
            $this->render('home', $data);
        }
	}
}

/* End of file welcome.php */
/* Location: ./application/controllers/welcome.php */