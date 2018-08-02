using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace StrReverse
{
    class Program
    {
        /// <summary>
        /// This method reverses the input string
        /// </summary>
        /// <param name="strInput">The string to be reversed</param>
        /// <param name="strResult">The [out] param which will contain the resulting reversed string if there was no error or empty string otherwise</param>
        /// <returns>Return string length on success and a negative value otherwise</returns>
        static int stringReverse(string strInput, out string strResult)
        {
            // Prepare the result
            strResult = string.Empty;

            // Pre-save the length
            var strLen = strInput.Length;

            // Check the input
            if (strInput == null || strLen == 0)
            {
                return -1;
            }

            // We'll use the string builder to avoid excessive
            // copying of the c# immutable strings. Also preallocate
            // the space to be large enough to capture the enitre
            // string without the need to re-allocate space
            StringBuilder sb = new StringBuilder(strLen);

            // Run the string backwards
            for(var i = strLen-1; i>=0; i--)
            {
                sb.Append(strInput[i]);
            }

            // Format the output
            strResult = sb.ToString();

            // Seems to be fine
            return 0;
        }

        static void printUsage()
        {
            // Grad the app's executable file name
            string exePath = System.Reflection.Assembly.GetEntryAssembly().Location;
            string exeName = System.IO.Path.GetFileName(exePath);

            Console.WriteLine("String reverse application.");
            Console.WriteLine("Usage:");
            Console.WriteLine(string.Format("{0} [string]", exeName));
            Console.WriteLine("[string] - (optional) The string to reverse. Please use double quotes for multi word strings.");
            Console.WriteLine();
        }

        static int Main(string[] args)
        {
            // If the app was started with commandline argument or not
            bool isInteractive = (args.Length == 0);

            // Prepare the input
            string strInput = string.Empty;

            if (isInteractive)
            {
                printUsage();
                Console.WriteLine("Please provide a string to reverse:");

                // Read the input
                strInput = Console.ReadLine();
            }
            else // Command line arguments
            {
                if(args.Length != 1)
                {
                    printUsage();
                    return -2;
                }

                // Get the input from command line
                strInput = args[0];
            }

            // Prepare the output
            string strReverse = String.Empty;

            var result = stringReverse(strInput, out strReverse);
            switch(result)
            {
                case 0:
                    if (isInteractive)
                    {
                        Console.WriteLine("The reversed string:"); // we're ok here
                    }
                    break;
                case -1:
                    Console.WriteLine("Please provide non-empty string");
                    return result;
                default:
                    Console.WriteLine("Unknown error");
                    return result;
            }

            // Print the result
            Console.WriteLine(strReverse);

            // We're done here
            return 0;
        }
    }
}
