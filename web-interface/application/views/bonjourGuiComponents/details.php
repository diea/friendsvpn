<div class="row">
<div class="col-xs-2" id="options">
<h2>Options</h2>
<button type="button" class="delete btn btn-danger">Delete service</button>
</div>
</div>
<div class="row">
<div class="col-xs-10" id="friends">
<h2>Current Permissions</h2>
    <div class="bs-callout bs-callout-success">
    <h4>Authorized users</h4>
    <div id="authorizedLinks">
            <?php
        foreach($friends_auth as $friend) {
            // left out $friend["uid"]
            ?>
            <a href="#" class="friendPic" data-uid="<?php echo $friend["uid"]; ?>"><span class="friendSquare friendAuthorized"><?php echo $friend["name"]; ?><img src="<?php echo $friend["pic_square"]; ?>"></span></a>
            <?php
        }
        ?>
    </div>
    </div>
    <div class="bs-callout bs-callout-danger">
    <div id="deniedLinks">
    <h4>Denied users</h4>
            <?php
        foreach($friends_denied as $friend) {
            // left out $friend["uid"]
            ?>
            <a href="#" class="friendPic" data-uid="<?php echo $friend["uid"]; ?>"><span class="friendSquare friendDenied"><?php echo $friend["name"]; ?><img src="<?php echo $friend["pic_square"]; ?>"></span></a>
            <?php
        }
        ?>
    </div>
    </div>
</div>
</div>

<script>
$(".friendPic").click(function(event) {
    event.preventDefault();
    var divChild = $(this).children().first();
    var friendPic = $(this);
    if (divChild.hasClass("friendAuthorized")) {
        // deauthorize
        $.ajax({
            url: "bonjourGui/deAuthorizeUser",
            type: "POST",
            data: {
                "service": $(".active.liservice").children().first().data("name"),
                "hostname": $(".active.lihost").children().first().html(),
                "name": $(".active.liport").children().first().data("name"),
                "port": $(".active.liport").children().first().data("port"),
                "transProt": $(".active.liport").children().first().data("transprot"),
                "uid": $(this).data("uid")
            }
        }).done(function() {
            // change class
            divChild.removeClass("friendAuthorized");
            divChild.addClass("friendDenied");
            friendPic.detach().appendTo("#deniedLinks");
        });
    } else {
        // authorize
        $.ajax({
            url: "bonjourGui/authorizeUser/",
            type: "POST",
            data: {
                "service": $(".active.liservice").children().first().data("name"),
                "hostname": $(".active.lihost").children().first().html(),
                "name": $(".active.liport").children().first().data("name"),
                "port": $(".active.liport").children().first().data("port"),
                "transProt": $(".active.liport").children().first().data("transprot"),
                "uid": $(this).data("uid")
            }
        }).done(function() {
            // change class
            divChild.removeClass("friendDenied");
            divChild.addClass("friendAuthorized");
            friendPic.detach().appendTo("#authorizedLinks");
        });
    }
});
var alertTimeout;
$(".delete").click(function(event) {
    event.preventDefault();
    // deauthorize
    serviceName = $(".active.liport").children().first().data("name");
    clearTimeout(alertTimeout);
    $.ajax({
        url: "bonjourGui/deleteRecord",
        type: "POST",
        data: {
            "service": $(".active.liservice").children().first().data("name"),
            "hostname": $(".active.lihost").children().first().html(),
            "name": $(".active.liport").children().first().data("name"),
            "port": $(".active.liport").children().first().data("port"),
            "transProt": $(".active.liport").children().first().data("transprot"),
            "uid": $(this).data("uid")
        }
    }).done(function() {
        // reload page
        //location.reload(); // refresh page
        $(".active.liservice").remove();
        $(".active.lihost").remove();
        $(".active.liport").remove();
        $(".permissionsField").html("");
        $("#otherAlerts").html('<div class="alert alert-info">The service ' + serviceName + ' has been deleted</div>');
        if ($("#unFold").css("display") == "block") {
            $("#unFold").css("display", "none");
            $(".liservice:not(.active)").toggle("fold");
        }

        alertTimeout = setTimeout(function() {
            $("#otherAlerts").html(""); // remove alert
        }, 10000);
    });
});

</script>