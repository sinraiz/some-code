using System;
using System.Collections.Generic;
using System.Text;

namespace PizzaApp.Data 
{
    public class TopPizzaData
    {
        public TopPizzaData()
        {
            toppings = new List<string>();
            ordersCount = 0;
        }
        public uint ordersCount { get; set; }
        public List<string> toppings { get; set; }

        //
        // Summary:
        //     Calculates the title of the pizza based on the toppings.
        //
        // Returns:
        //     Returns a string with 1, 2 or 3 topping contained in the
        //     pizza. 
        //
        // Remarks:
        //     The actual number of toppings listed depends on how many 
        //     of toppings this pizza has and we can pick 3 of them
        public string title
        {
            get
            {
                string titleRes = string.Empty;

                if (toppings.Count < 2)
                {
                    titleRes = toppings[0];
                }
                else
                if (toppings.Count < 3)
                {
                    titleRes = string.Format("{0} with {1}", toppings[0], toppings[1]);
                }
                else
                if (toppings.Count >= 3)
                {
                    titleRes = string.Format("{0}, {1} and {2}", toppings[0], toppings[1], toppings[2]);
                }

                return System.Globalization.CultureInfo.CurrentCulture.TextInfo.ToTitleCase(titleRes.ToLower());
            }
        }

        //
        // Summary:
        //     Identifies if the row corresponding to this object
        //     must be presented expanded or collapsed.
        //
        // Returns:
        //     Returns true if the row must be expanded and false
        //     otherwise
        //
        // Remarks:
        //     The TopPizzaData bound to the list cells are collapsed
        //     by default. User actions can toggle the row view and this
        //     gets persisted into the isExpanded property
        private bool _isExpanded;
        public bool isExpanded
        {
            get
            {
                return _isExpanded;
            }
            set
            {
                _isExpanded = value;
                needsRedraw = true;
            }
        }

        //
        // Summary:
        //     Identifies if the row's expand/collapse
        //     button needs to be animated
        //
        // Remarks:
        //     This one is automatically set to true
        //     whenever the isExpanded was toggled
        public bool needsRedraw { get; set; }
    }
}
