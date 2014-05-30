<div class="row">
<div class="col-xs-2" id="options">
<h2>Options</h2>
<button type="button" class="delete btn btn-danger">Delete record</button>
</div>
</div>
<div class="row">
<div class="col-xs-10" id="friends">
<h2>Current Permissions</h2>

        <?php
    foreach($friends as $friend) {
        // left out $friend["uid"]
        ?>
        <a href="#" class="friendPic" data-uid="<?php echo $friend["uid"]; ?>"><div class="friendSquare <?php if ($friend["authorized"] == true) echo "friendAuthorized"; else echo "friendDenied"; ?>"><?php echo $friend["name"]; ?><img src="<?php echo $friend["pic_square"]; ?>"></div></a><br>
        <?php
    }
    ?>
</div>


</div>

<script>
$(".friendPic").click(function(event) {
    event.preventDefault();
    var divChild = $(this).children().first();
    if (divChild.hasClass("friendAuthorized")) {
        // deauthorize
        $.ajax({
            url: "/bonjourGui/deAuthorizeUser",
            type: "POST",
            data: {
                "service": $(".active.liservice").children().first().html(),
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
        });
    } else {
        // authorize
        $.ajax({
            url: "/bonjourGui/authorizeUser",
            type: "POST",
            data: {
                "service": $(".active.liservice").children().first().html(),
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
        });
    }
});
$(".delete").click(function(event) {
    event.preventDefault();
    // deauthorize
    $.ajax({
        url: "/bonjourGui/deleteRecord",
        type: "POST",
        data: {
            "service": $(".active.liservice").children().first().html(),
            "hostname": $(".active.lihost").children().first().html(),
            "name": $(".active.liport").children().first().data("name"),
            "port": $(".active.liport").children().first().data("port"),
            "transProt": $(".active.liport").children().first().data("transprot"),
            "uid": $(this).data("uid")
        }
    }).done(function() {
        // reload page
        location.reload(); // refresh page
    });
});

</script>