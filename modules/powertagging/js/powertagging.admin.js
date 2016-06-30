(function ($) {
Drupal.behaviors.powertagging = {
  attach: function (context) {

    if ($("fieldset#edit-batch-jobs").length > 0) {
      $("input#edit-batch-jobs-index").click(function(e) {
        e.preventDefault();
        
        $(this).siblings("table").children("tbody").find("input:checkbox:checked").each(function() {
          $(this).attr("checked", false);
          $(this).closest("table").children("thead").find("input:checkbox:checked").attr("checked", false);
          $(this).closest("tr").removeClass("selected");

          // Check if indexing or synchronization of this PowerTagging
          // configuration already runs.
          $.getJSON(Drupal.settings.basePath + 'powertagging/index', function(result_data) {
            if (result_data.success) {
              console.log(result_data.message);
            }
            else {
              console.log(result_data.message);
            }
          });
        });
      });
    }

    // Make the project tables sortable if tablesorter is available.
    if ($.isFunction($.fn.tablesorter)) {
      $("table#powertagging-configurations-table").tablesorter({
        widgets: ["zebra"],
        widgetOptions: {
          zebra: ["odd", "even"]
        },
        sortList: [[0, 0]],
        headers: {
          3: {sorter: false},
          4: {sorter: false}
        }
      });
    }

  }
};
})(jQuery);
