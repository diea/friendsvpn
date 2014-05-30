<?php if ( ! defined('BASEPATH')) exit('No direct script access allowed');

class MY_Controller extends CI_Controller {
    protected $layout = 'layouts/default';

    public function __construct()
    {
        parent::__construct();
    }

    protected function render($file = NULL, &$viewData = array(), $layoutData = array())
    {
        if( !is_null($file) ) {
            $data['page'] = $file;
            $data['content'] = $this->load->view($file, $viewData, TRUE);
            $data['layout'] = $layoutData;
            $this->load->view($this->layout, $data);
        } else {
            $this->load->view($this->layout, $viewData);
        }

        $viewData = array();
    }
}