using System;
using System.Drawing;

using CoreFoundation;
using UIKit;
using Foundation;
using CoreGraphics;

namespace PizzaApp
{
    [Register("UniversalView")]
    public class UniversalView : UIView
    {
        UILabel label1;
        public UniversalView()
        {
            Initialize();
        }

        public UniversalView(RectangleF bounds) : base(bounds)
        {
            Initialize();
        }

        void Initialize()
        {
            BackgroundColor = UIColor.Green;

            

            var frame = new CGRect(10, 10, 300, 30);
            label1 = new UILabel(frame);
            label1.Text = "New Label";
            this.Add(label1);
        }
    }

    [Register("MainViewController")]
    public class MainViewController : UIViewController
    {
        public MainViewController()
        {
        }

        public override void DidReceiveMemoryWarning()
        {
            // Releases the view if it doesn't have a superview.
            base.DidReceiveMemoryWarning();

            // Release any cached data, images, etc that aren't in use.
        }


        public override void ViewDidLoad()
        {
            View = new UniversalView();

            base.ViewDidLoad();
            
        }
    }
}