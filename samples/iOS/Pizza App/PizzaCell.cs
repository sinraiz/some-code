using Foundation;
using System;
using System.Collections.Generic;
using System.Text;
using UIKit;

namespace PizzaApp
{
    public partial class PizzaCell : UITableViewCell
    {
        public PizzaCell(IntPtr handle) : base(handle)
        {
            isExpanded = false;
            parent = null;
        }

        public void initCell()
        {
           /* btnExpandCollapse.TouchUpInside += (s, e) =>
            {
                BtnExpandCollapse_TouchUpInside(btnExpandCollapse);
            };*/
        }



        public string title
        {
            get
            {
                return lblPizzaTitle.Text;
            }
            set
            {
                lblPizzaTitle.Text = value;
            }
        }

        public uint ordersCount
        {
            get
            {
                uint resValue = 0;
                if (!uint.TryParse(lblOrdersCount.Text, out resValue))
                    return 0;
                return resValue;
            }
            set
            {
                lblOrdersCount.Text = value.ToString();
            }
        }
        
        public bool isExpanded { get; set;}

        public UITableView parent { get; set; }

        private NSIndexPath indexPath
        {
            get
            {
                if (parent == null)
                    return null;
                return parent.IndexPathForCell(this);
            }
        }

        /*partial void BtnExpandCollapse_TouchUpInside(UIButton sender)
        {
            this.isExpanded = !this.isExpanded;
            var frame = this.ContentView.Frame;
            frame.Height = 200;
            this.ContentView.Frame = frame;

            if (indexPath != null)
            {
                // Refresh oneself to recalculate own row's
                // height based on expanded/collapsed state
                parent.ReloadRows(new NSIndexPath[] { indexPath }, 
                                UITableViewRowAnimation.Bottom);
            }
        }*/
        
    }
}
