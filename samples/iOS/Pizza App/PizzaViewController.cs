
using System;
using System.Drawing;

using Foundation;
using UIKit;
using System.Collections.Generic;
using System.Linq;

namespace PizzaApp
{
    public partial class PizzaViewController : UIViewController
    {
        public PizzaViewController(IntPtr handle) : base(handle)
        {
        }

        public override void ViewDidLoad()
        {
            base.ViewDidLoad();

            tvPizzaList.RowHeight = UITableView.AutomaticDimension;
            tvPizzaList.EstimatedRowHeight = 60;

            tvPizzaList.Source = new PizzaTableViewSource();
            tvPizzaList.ReloadData();
        }
        public override void ViewWillAppear(bool animated)
        {
            
        }

        public override void DidReceiveMemoryWarning()
        {
            base.DidReceiveMemoryWarning();
            // Release any cached data, images, etc that aren't in use.
        }
    }



}