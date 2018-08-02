
using System;
using System.Drawing;

using Foundation;
using UIKit;
using PizzaApp.Data;

namespace PizzaApp
{
    public partial class MainViewController : UIViewController
    {
        public MainViewController(IntPtr handle) : base(handle)
        {
        }

        public override void DidReceiveMemoryWarning()
        {
            // Releases the view if it doesn't have a superview.
            base.DidReceiveMemoryWarning();

            // Release any cached data, images, etc that aren't in use.
        }
        public override Boolean ShouldAutorotate()
        {
            return true;
        }

        #region View lifecycle

        public override void ViewDidLoad()
        {
            base.ViewDidLoad();

            // The loading overlay
            pnlLoadingOverlay.BackgroundColor = UIColor.FromPatternImage(new UIImage("pizza-bg-pattern.png"));
            pnlLoadingOverlay.Hidden = false;
            pbProgress.Progress = 0;

            // Start loading the data
            var rawData = new PizzaDataset();
            rawData.loadData(
                // Loading progress callback
                (bytesRead, bytesTotal) =>
                {
                    float percentage = (float)((double)bytesRead / (double)bytesTotal);

                    // Update the UI
                    InvokeOnMainThread(() =>
                    {
                        pbProgress.Progress = percentage;
                    });
                },
                // Finish callback
                (success) =>
                {
                    if (!success)
                        return;

                    var displayData = new PizzaTableViewSource(rawData);

                    // Update the UI
                    InvokeOnMainThread(() => 
                    {
                        // hide the progress bar
                        pnlLoadingOverlay.Hidden = true;

                        // set the table data
                        tvPizzaList.Source = displayData;
                     tvPizzaList.ReloadData();
                    });

                }
            );
            

            // Update the header icon
            updateExpandAllIcon();

            // Setup the TableView decoration
            //tvPizzaList.BackgroundColor = UIColor.FromRGB(34, 35, 40);
            tvPizzaList.BackgroundColor = UIColor.FromPatternImage(new UIImage("pizza-bg-pattern.png"));

            // Setup the TableView data
            tvPizzaList.RowHeight = UITableView.AutomaticDimension;
            tvPizzaList.EstimatedRowHeight = 60;
        }

        public override void ViewWillAppear(bool animated)
        {
            base.ViewWillAppear(animated);
        }

        public override void ViewDidAppear(bool animated)
        {
            base.ViewDidAppear(animated);
        }

        public override void ViewWillDisappear(bool animated)
        {
            base.ViewWillDisappear(animated);
        }

        public override void ViewDidDisappear(bool animated)
        {
            base.ViewDidDisappear(animated);
        }

        #endregion


        private bool allExpanded = false;

        private void updateExpandAllIcon()
        {
            UIImage tintedImage = new UIImage(allExpanded ? "collapse-all.png" : "expand-all.png");

            // to remove the tint
            //var rawImage = rawImage.ImageWithRenderingMode(UIImageRenderingMode.AlwaysOriginal);

            btnExpandAll.Image = tintedImage;
        }

        partial void BtnExpandAll_Activated(UIBarButtonItem sender)
        {
            var dataset = (tvPizzaList.Source as PizzaTableViewSource);
            if(dataset == null)
            {
                return;
            }

            // Switch the behaviour
            allExpanded = !allExpanded;

            // Iterate all the pizza items and expand them
            foreach (var pizza in dataset.topPizzas)
            {
                pizza.isExpanded = allExpanded;
            }

            // Refresh the table
            tvPizzaList.ReloadData();

            // Update the header icon
            updateExpandAllIcon();
        }
    }
}