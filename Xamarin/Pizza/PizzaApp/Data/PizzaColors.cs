using System;
using System.Collections.Generic;
using System.Text;
using UIKit;

namespace PizzaApp.Data
{
    public class PizzaColors
    {
        //
        // Summary:
        //     Peeks a color for the topping from a natural palette 
        //
        // Parameters:
        //   topping:
        //     The title of the topping to get the color for
        //
        // Remarks:
        //     This method guarantees that every topping will get the
        //     same color for every call. No thread-safety though.
        //
        // Returns:
        //     Returns the UI color for the given topping
        public static UIColor getColor(string topping)
        {
            var toppingName = topping.ToLower();

            if(!assigned_colors.ContainsKey(toppingName))
            {
                // Make use of a mod to cycle the colors
                int newColorIdx = assigned_colors.Count % palette.Length;
                assigned_colors[toppingName] = palette[newColorIdx];
            }

            // Now we are guaranteed to have the color assigned
            uint colorRaw = assigned_colors[toppingName];

            // Convert to GUI
            return UIColor.FromRGB(
                (byte)((colorRaw & 0xFF0000) >> 16), // red
                (byte)((colorRaw & 0x00FF00) >> 08), // green
                (byte)((colorRaw & 0x0000FF) >> 00)  // blue
                );
        }

        // The colors or nature, the colors
        // of pizza
        private static uint [] palette =
        {
            0xde5d57,
            0xe7c15c,
            0x222328,
            0x8da04f,

            0x95383b,
            0xbf1e2d,
            0x633e60,
            0xefdc06,

            0x533d30,
            0xf7941d,
            0xda833c,
            0x3e6e3e,

            0xdace48,
            0xff8f55,
            0xffbd29,
            0xda0501
        };

        // Keeps track of all the earlier 
        // assigned topping colors
        private static Dictionary<string, uint> assigned_colors = new Dictionary<string, uint>();

    }
}
