<?php
if (empty($_SERVER['HTTPS'])) {
?>
    <div class="alert alert-danger">
        You are not using a secure protocol to access this application. The certificate to access the secure version can be downloaded <a href="<?php echo base_url(APPPATH . '/assets/localhostssl.crt'); ?>">here</a>
    </div>
<?php
}
?>
<?php
if (isset($needLogin)) {
    ?>
    <div class="row">
        <div class="col-md-4 col-md-offset4">
            <div class="alert alert-info">
                <p> It seems you are not logged in to facebook </p>
            </div>
            <a class="btn btn-primary" href="<?php echo $url; ?>">Log in</a>
        </div>
    </div>
    <?php
} else {
    ?>
    
<div id="appNotRunningDiv">
    <div class="alert alert-warning">
        Your desktop app hasn't processed recent XML rpc requests, either it is not running or there is some delay.
    </div>
</div>
<div id="noservices">
    <div class="alert alert-info">
        You don't have any services yet. Launch the desktop application and refresh this page to see your services.
    </div>
</div>
<div class="row">
    <div class="col-xs-2">
        <div id="servicesList">
        </div>
    </div>
    <div class="col-xs-3" id="hostsList">
    </div>
    <div class="col-xs-7">
        <div class="row">
            <div id="serviceNamePortList"></div>
            <div class="permissionsField"></div>
        </div>
    </div>
    </div>
</div>
<div class="row">
    <div class="col-xs-offset-5 col-xs-7">
    </div>
</div>



<div class="row">
    <div class="col-md-12 specialProtocolFeatures">
        <!-- TODO -->
    </div>
</div>
    <script>
    /**
     * Returns true if request_id for uid has been processed (and thus appIsRunning)
     */
    function appIsRunning(request_id, uid) {
        $.getJSON("xmlrpc/requestProcessed/" + request_id, function(data) {
            var appIsRunning = data.request_processed;
            if (!appIsRunning) {
                $("#appNotRunningDiv").css("display", "block");
            } else {
                $("#appNotRunningDiv").css("display", "none");
                window.clearInterval(appIsRunningTimer);
                window.clearTimeout(testUidParserTimer);
                testUidParserTimer = window.setTimeout("testUidParsed()", 30000);
            }
        });
    }
    /**
     * Will test that application is running every 2 seconds by
     * issuing a setUid XMLRPC call and see if it has been parsed.
     */
    function testUidParsed() {
        $.ajax({
            url: "xmlrpc/setUid/<?php echo $userId; ?>"
        }).done(function(data) {
            var request_id = $.parseJSON(data).request_id;
            appIsRunningTimer = setInterval("appIsRunning(" + request_id + ", <?php echo $userId; ?>)", 10000);
        });
    }
    /* send xml rpc to give user id to desktop client */
    testUidParserTimer = window.setTimeout("testUidParsed()", 10000);
    
    /* Display the bonjour browser */
    $.ajax({
        url: "bonjourgui/services/"
    }).done(function(data) {
        $("#servicesList").html(data);
        if ($(".serviceli").length == 0) {
            $("#noservices").css("display", "block");
        } else {
            $("#helpmenu").css("display", "block");
        }
    });
    
    </script>
    
    <?php
}
?>
