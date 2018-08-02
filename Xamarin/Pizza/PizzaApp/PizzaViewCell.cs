using CoreGraphics;
using Foundation;
using PizzaApp.Data;
using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Globalization;
using UIKit;

namespace PizzaApp
{
	partial class PizzaViewCell : UITableViewCell
	{
		public PizzaViewCell (IntPtr handle) : base (handle)
        {
            parent = null;
        }

        public override void LayoutSubviews()
        {
            base.LayoutSubviews();
            fillToppingsAndMeasure(false);
        }

        partial void BtnExpand_TouchUpInside(UIButton sender)
        {
            // Toggle the expanded flag inside
            // the underlying data
            data.isExpanded = !data.isExpanded;

            // Animate the arrow button
            rotateButton();

            // Hide the toppings first
            if(!data.isExpanded)
            {
                recycleAllToppingLabels();
            }

            // Call the tableview to redraw this cell
            if (indexPath != null)
            {
                // Refresh oneself to recalculate own row's
                // height based on expanded/collapsed state
                parent.ReloadRows(new NSIndexPath[] { indexPath },
                                UITableViewRowAnimation.None);
            }

        }

        //
        // Summary:
        //     Rotate the expand/collapse button based on its state.
        //
        // Parameters:
        //   tableView:
        //     fromLeft [false] If the rotation is done clockwise
        //
        //   duration:
        //     Animation speed.
        //
        //   onFinished:
        //     Completion callback.
        //
        // Remarks:
        //     This method is called by the tableview to perform 
        //     spinning of theexpand/collapse button upon the 
        //     user's clicks on it
        public void rotateButton(bool fromLeft = false, double duration = 0.3, Action onFinished = null)
        {
            bool toUp = data.isExpanded;


            var downTransform = CGAffineTransform.MakeRotation((nfloat)(2 * Math.PI));
            var upTransform = CGAffineTransform.MakeRotation((nfloat)((fromLeft ? 1 : -1) * Math.PI));

            var sourceTransform = toUp ? downTransform : upTransform;
            var targetTransform = toUp ? upTransform : downTransform;
            
            btnExpand.Transform = sourceTransform;
            UIView.Animate(duration, 0, UIViewAnimationOptions.CurveEaseInOut,
                () => {
                    btnExpand.Transform = targetTransform;
                },
                onFinished
            );
        }

        //
        // Summary:
        //     Peeks a label UI object for the topping from cache or creates
        //     it if the cache is empty 
        //
        // Parameters:
        //   text:
        //     The title of the topping to get the label for
        //
        // Remarks:
        //     This method makes use of two stacks to peek a label object from
        //     one of them (cache) modify accordingly to the topping and push
        //     into the stack of visible toppings
        //
        // Returns:
        //     Returns the UI label object from cache or a brand new one. It has
        //     the right style applied and the text is set to the given topping
        private UILabel getToppingLabel(string text)
        {
            // Check if we have any free label in the cache of labels
            UILabel result = null;

            if(_toppingLabels.Count > 0)
            {
                result = _toppingLabels.Pop();
            }
            else
            {
                // no labels available, so create one
                var frame = new CGRect(0, 0, 50, 15);
                result = new UILabel(frame);
                result.Font = UIFont.SystemFontOfSize(12, UIFontWeight.Regular);
                result.TextAlignment = UITextAlignment.Center;
                result.TextColor = UIColor.White;
            }
            // Assign the text
            result.Text = text;

            // Calculate the color for the topping
            // Set the label color and shape
            result.Layer.BackgroundColor = PizzaColors.getColor(text).CGColor; 
            result.Layer.CornerRadius = 4;
            result.Layer.Opacity = 1;

            // Push the resulting label onto the stack
            _visibleToppingLabels.Push(result);

            return result;
        }

        //
        // Summary:
        //     Returns the label UI object once used for a topping into the
        //     cache
        //
        // Parameters:
        //   label:
        //     The label previously used to display one of the toppings
        //
        // Remarks:
        //     This method returns the given label into the stack caching the 
        //     unused label items while removing it at the same time from the 
        //     stack with visible topping labels
        //
        private void cacheToppingLabel(UILabel label)
        {
            // Push the resulting label into the cache
            _toppingLabels.Push(label);

            label.RemoveFromSuperview();
        }

        //
        // Summary:
        //     Returns all previously used topping labels into the cache
        //
        private void recycleAllToppingLabels()
        {
            // Clone the collection
            var clone = new Stack<UILabel>(_visibleToppingLabels);

            // Empty the used set
            _visibleToppingLabels.Clear();

            // Put all of them into the cache
            foreach (var label in clone)
            {
                cacheToppingLabel(label);
            }
        }

        //
        // Summary:
        //     Calculates the dimensions of the text of given font
        //
        // Parameters:
        //   font:
        //     The font used to draw the text
        //   text:
        //     The text string to be measured
        //   height:
        //     The max. height of the text. Single line
        //
        // Remarks:
        //     This method emulates drawing the text string with one single
        //     line and return the width and height sufficient to accomodate
        //     the given string draw with the given font
        //
        // Returns:
        //     Returns the bounding rectangle for drawing the given string
        //
        private CGSize measureText(UIFont font, string text, nfloat height)
        {
            var nsText = new NSString(text);
            var boundSize = new CGSize(float.MaxValue, height);
            var options = NSStringDrawingOptions.UsesFontLeading |
                NSStringDrawingOptions.UsesLineFragmentOrigin;

            var attributes = new UIStringAttributes
            {
                Font = font
            };

            var sizeF = nsText.GetBoundingRect(boundSize, options, attributes, null).Size;

            return sizeF;
        }

        //
        // Summary:
        //     The function serves two purposes: calculate the row height
        //     sufficient to display all the toppings and secondly add the
        //     topping labels into the row in the form of rounded label tags
        //
        // Parameters:
        //   onlyMeasure:
        //     Defines if we only need to know the height for such a layout
        //     or we also want to add the topping labels into the layout
        //
        // Remarks:
        //     This method is called by the tableview to perform the row height
        //     calculation and also by the cell itself when it's in the expanded
        //     state and needs to show the toppings
        public nfloat fillToppingsAndMeasure(bool onlyMeasure)
        {
            // If we really adding the topping labels
            // then first remove all the previously used
            if (!onlyMeasure)
            {
                recycleAllToppingLabels();
            }

            // If the row is collapsed then the height
            // is already known
            if (!data.isExpanded)
            {
                return btnExpand.Frame.Bottom + UIConst.ROW_SPACING;
            }


            // Consider the available width inside the row
            float lineWidth = (float)(this.parent.Bounds.Width 
                - lblPizzaTitle.Bounds.Left
                // - lblOrdersCount.Bounds.Width                   // Uncomment
                // - UIConst.ROW_SPACING - btnExpand.Bounds.Width  // to make it narrow
                - UIConst.ROW_SPACING
                );

            // Let's start with the expand button's end
            // and then move right and below wrapping the
            // layout
            CGRect lastLabelPosition = btnExpand.Frame;

            // Label used for the text measurement
            UILabel label = getToppingLabel("Dummy"); 

            // Create labels for every topping listed
            foreach (var _topping in data.toppings)
            {
                // Beautify the topping name
                var topping = CultureInfo.CurrentCulture.TextInfo.ToTitleCase(_topping.ToLower());

                // Fetch the label from cache but
                // only if we are really about to add 
                // it to the layout
                if (!onlyMeasure)
                {
                    label = getToppingLabel(topping);
                }

                // Calculate the size required for the
                // topping's title text
                var sizeNeeded = measureText(label.Font, topping, label.Frame.Height);

                // It shall not be longer then the cell width for sure
                sizeNeeded.Width = Math.Min((float)sizeNeeded.Width, lineWidth);

                // Give some extra left/right padding
                sizeNeeded.Width += (2 * UIConst.H_SPACE);

                // How much space is left in the row
                nfloat remainingWidth = lineWidth - UIConst.H_SPACE - (float)lastLabelPosition.Right;

                // Check if the label fits the row
                if(remainingWidth < sizeNeeded.Width)
                {
                    // if won't fit so move to the next line
                    lastLabelPosition.X = lblPizzaTitle.Frame.X;
                    lastLabelPosition.Y = lastLabelPosition.Bottom + UIConst.V_SPACE;
                }
                else
                {
                    // Move to the right
                    lastLabelPosition.X = lastLabelPosition.Right + UIConst.H_SPACE;
                }

                // Set the new width
                lastLabelPosition.Width = sizeNeeded.Width;

                // Set the compactness
                lastLabelPosition.Height = sizeNeeded.Height + 2 * UIConst.V_SPACE;

                if (!onlyMeasure)
                {
                    // Position the label
                    label.Frame = lastLabelPosition;

                    // Show it
                    this.Add(label);
                }
            }

            // If we were just measuring then we used only one label
            // let's reuse it
            if (onlyMeasure)
            {
                cacheToppingLabel(label);
            }

            // The last label's bottom defines the required height
            return lastLabelPosition.Bottom + UIConst.ROW_SPACING;
        }

        #region Data Related Things

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

        public Data.TopPizzaData data { get; set; }

        #endregion

        #region UI Aux Stuff
        //
        // Summary: The reference to parent table is required to
        //          make the tableview update this row upon the click
        public UITableView parent { get; set; }

        //
        // Summary: We might need to know this row's current index
        private NSIndexPath indexPath
        {
            get
            {
                if (parent == null)
                    return null;
                return parent.IndexPathForCell(this);
            }
        }

        //
        // Summary: We keep a cache of the topping UI label object
        //          to be able to reuse them efficiently rather than
        //          create them everytime from scratch
        private Stack<UILabel> _toppingLabels = new Stack<UILabel>();

        //
        // Summary: We keep a cache of the topping UI label object
        //          to be able to reuse them. This colelction keeps
        //          those labels from _toppingLabels which are currently
        //          shown
        private Stack<UILabel> _visibleToppingLabels = new Stack<UILabel>();
        #endregion
    }

    //
    // Summary:
    //     Here we keep some numbers used in the row's layout
    public static class UIConst
    {
        // The padding between the major blocks of the row
        public const float ROW_SPACING = 16;

        // Vertical spacing between the topping labels
        public const float V_SPACE = 3;

        // Horizontal spacing between the topping labels
        public const float H_SPACE = 3;
    }
}
