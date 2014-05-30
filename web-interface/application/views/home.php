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
<div class="row">
    <div class="col-xs-2" id="servicesList">
    </div>
    <div class="col-xs-2" id="hostsList">
    </div>
    <div class="col-xs-5">
        <div class="row">
            <div id="serviceNamePortList"></div>
            <div class="permissionsField"></div>
        </div>
    </div>
    <div class="col-xs-3">
    <div class="alert">
    <div class="friendSquare friendAuthorized">A user who is inside this box is authorized.</div>
    <br>
    <div class="friendSquare friendDenied ">A user who is inside this box is denied from using your service.</div>
    </div>
    
    <button type="button" id ="deleteAll" class="btn btn-danger">Clean up all records</button>
    
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
    });
    
    </script>
    
    <?php
}
?>
<script>
$("#deleteAll").click(function(event) {
    event.preventDefault();
    // deauthorize
    $.ajax({
        url: "/bonjourGui/deleteAll",
        type: "GET",
    }).done(function() {
        // reload page
        location.reload(); // refresh page
    });
});
</script>
