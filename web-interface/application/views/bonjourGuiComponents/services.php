<ul class="nav nav-pills nav-stacked">
  <?php
if ($services) {
  foreach($services as $service) {
      echo "<li class=\"liservice\">";
      ?>
      <a href="#" class="serviceli"><?php echo $service["name"]; ?></a>
      <?php
      echo "</li>";
  }
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
        
        // remove before reloading necessary things
        $("#serviceNamePortList").html("");
        $("#hostsList").html("");
        $(".permissionsField").html("")
        $(".specialProtocolFeatures").html("");
        
        var service = $(this).html();
        $.ajax({
            type: "POST",
            url:"bonjourgui/hostnames/",
            data: {
                "service": service
            }
        }).done(function(data) {
            $("#hostsList").html(data);
        });
    });
</script>