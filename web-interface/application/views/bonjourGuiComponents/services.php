<ul class="nav nav-pills nav-stacked">
  <?php
if ($services) {
  foreach($services as $service) {
      echo "<li class=\"liservice\">";
      ?>
      <a href="#" data-name="<?php echo $service["name"]; ?>" class="serviceli"><?php echo $service["fancy_name"]; ?></a>
      <?php
      echo "</li>";
  }
  ?>
  <li id="unFold" style="display:none;"><a href="#">...</a></li>
  <br>
  <button type="button" id ="deleteAll" class="btn btn-danger">Clean up all services</button>
  <?php
}
  ?>
</ul>

<script>
/* ajax call for the services offered by a given host */
$(".serviceli").click(function(event) {
    event.preventDefault();
    //alert($(this).html());
    $(".liservice").removeClass("active");
    $(this).parent().addClass("active");
    
    $(".liservice:not(.active)").toggle("fold");
    $("#unFold").css("display", "block");
    
    // remove before reloading necessary things
    $("#serviceNamePortList").html("");
    $("#hostsList").html("");
    $(".permissionsField").html("")
    $(".specialProtocolFeatures").html("");
    
    var service = $(this).data("name");
    $.ajax({
        type: "POST",
        url:"bonjourGui/hostnames/",
        data: {
            "service": service
        }
    }).done(function(data) {
        $("#hostsList").html(data);
    });
});

$("#unFold").click(function(event) {
    event.preventDefault();
    $(".liservice:not(.active)").toggle("fold");
    $("#unFold").css("display", "none");
});
    
$("#deleteAll").click(function(event) {
    event.preventDefault();
    // deauthorize
    if(window.confirm("This will delete all your captured services, are you sure?")) {
        $.ajax({
            url: "bonjourGui/deleteAll",
            type: "GET",
        }).done(function() {
            // reload page
            location.reload(); // refresh page
        });
    }
});
</script>