using Newtonsoft.Json;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PizzaApp.Data
{
    //
    // Summary:
    //     Reads the source JSON using the incremental loading. This is done
    //     to save the memory and also to be able to report the parsing progress.
    //
    //     This solution moves along the file and parses the data objects (pizzas)
    //     one by one instead of reading the whole file at once.
    //  
    //     As the pizza toppings in the original dataset are not guarantied to be 
    //     listed in the same order even for identical pizzas there's a need to 
    //     intoduce a sort of a bit map indicating the inclusion of all the toppings
    //     into a pizza by means of boolean flags. Because the data is read only once 
    //     these bitmaps will usually be longer for the pizzas met later in the list
    //     so when comparing the keys of different lengths we pad the shorter ones 
    //     with trailing zeros
    //
    public class PizzaDataset
    {
        public PizzaDataset()
        {
            rawData = new Dictionary<PizzaSignature, TopPizzaData>(new PizzaEqualityComparer());
            uniqueToppings = new Dictionary<string, int>();
        }

        //
        // Summary:
        //     Reads the source JSON and build a collection of top 20 pizzas
        //
        // Parameters:
        //   progressCallback:
        //     Optional callback action called once in a while to update the loading
        //     progress 
        //
        //   completionCallback:
        //     Optional callback action triggered after the data has been processed
        //
        // Remarks:
        //     This method is just a wrapper for loadDataInternal and can be controlled
        //     both via a task which it returns and by the completionCallback
        //
        // Returns:
        //     Returns a task that can be used to track its completion
        public Task<bool> loadData(Action<long, long> progressCallback, Action<bool> completionCallback)
        {
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();

            Task.Factory.StartNew(() =>
            {
                bool res = loadDataInternal(progressCallback);

                // Finish the task
                tcs.SetResult(res);

                // Notify the caller
                if (completionCallback != null)
                {
                    completionCallback.Invoke(res);
                }
            });

            return tcs.Task;                
        }

        //
        // Summary:
        //     Internal methid that opens the JSON file and incrementally moves along
        //     this file parsing individual pizza records and placing them inside a
        //     a container where each set of toppings is kept only once alogn with the
        //     statics of the pizzas containing these toppings
        //
        // Parameters:
        //   progressCallback:
        //     Optional callback action called once in a while to update the loading
        //     progress 
        //
        // Returns:
        //     Returns TRUE if everything was fine or false otherwise
        private bool loadDataInternal(Action<long, long> progressCallback)
        { 
            // Clear the storage
            rawData.Clear();

            // Prepare a helper
            JsonSerializer serializer = new JsonSerializer();

            using (StreamReader sr = new StreamReader("pizzas.json"))
            {
                int recordsScanned = 0;

                using (JsonReader reader = new JsonTextReader(sr))
                {
                    if (!reader.Read() || reader.TokenType != JsonToken.StartArray)
                    { 
                        Console.WriteLine("Expected start of array of pizzas");
                        return false;
                    }

                    // Start reading the file incrementally. For this we'll be
                    // using a conbination of token reading by the reader object
                    // and once we encounter the pizza object start we'll use the
                    // normal serializer to read it. 
                    while (reader.Read())
                    {
                        // If we reached the end of array outside of a
                        // pizza object it means that it's the end of the 
                        // global array of pizzas, so return
                        if (reader.TokenType == JsonToken.EndArray)
                        {
                            // end of outer array, it's ok
                            // It means we're done
                            break;
                        }

                        // Check if it's an object. If not then something
                        // is wrong here
                        if (reader.TokenType != JsonToken.StartObject)
                        {
                            Console.WriteLine("Expected start of the pizza object");
                            return false;
                        }
                        
                        // The pizza object is found, parse it (only one object at a time)
                        var pizzaData = serializer.Deserialize<PizzaDataRaw>(reader);

                        // Prepare to hold the pizza signature
                        PizzaSignature pizzaSignature = new PizzaSignature(uniqueToppings.Count + 1);

                        // Read all of the pizza toppings and make this combination into
                        // a bit array used for the arregated storage
                        foreach(string topping in pizzaData.toppings)
                        {
                            // The index of topping in the bit array key
                            int toppingBitIndex = uniqueToppings.Count;

                            // Check if we met this topping before
                            if(!uniqueToppings.TryGetValue(topping, out toppingBitIndex))
                            {
                                // Save the topping into the known toppings set
                                uniqueToppings.Add(topping, uniqueToppings.Count);

                                toppingBitIndex = uniqueToppings.Count - 1;
                            }

                            // Check if the bit array has enough places. If not
                            // extend it and padd with zeros
                            while(toppingBitIndex >= pizzaSignature.data.Count)
                            {
                                pizzaSignature.data.Add('0');
                            }

                            // Set the flag for this topping in the signature
                            pizzaSignature.data[toppingBitIndex] = '1';
                        }

                        // Save the found toppings configuration
                        // Check if we met this combination before
                        TopPizzaData knownPizza = null;
                        if (!rawData.TryGetValue(pizzaSignature, out knownPizza))
                        {
                            // It's the first one of its kind
                            knownPizza = new TopPizzaData();
                            knownPizza.toppings = pizzaData.toppings;
                            knownPizza.ordersCount = 0;

                            // Save the pizza for the future. 
                            rawData[pizzaSignature] = knownPizza;
                        }

                        // Increment the counters
                        knownPizza.ordersCount = knownPizza.ordersCount + 1;

                        // From time to time report the progress
                        if((recordsScanned++ % 50) == 0)
                        {
                            // Notify the caller
                            if (progressCallback != null)
                            {
                                // Report the current progress
                                progressCallback(sr.BaseStream.Position, sr.BaseStream.Length);
                            }
                        }

                       // list.Add(pizzaData);
                    }
                }
            }

            // OMG!!!
            //var subset = topData;
            //var test = (from t in rawData
            //            where t.Value.toppings.Contains("sliced roma tomatoes")
            //            //orderby t.Delivery.SubmissionDate
            //            select t.Value).ToList();//.Take(5);

            return true;
        }

        //
        // The collection of the unique toppings used in
        // in all the pizzas. Required to build a sustainable 
        // bitmap used as a key for pizzas matching
        private Dictionary<string, int> uniqueToppings = null;

        // 
        // The raw data read into the collection mapping
        // toppings to the pizzas statistics
        private Dictionary<PizzaSignature, TopPizzaData> rawData = null;

        // 
        // The dynamic property using LINQ to fetch the
        // top twenty items from the rawData 
        public List<TopPizzaData> topData
        {
            get
            {
                List<TopPizzaData> result = new List<TopPizzaData>();
                                            

                var subset = rawData.
                    Select(i => i.Value)
                    .OrderByDescending(value => value.ordersCount)
                    .Take(20)
                    .ToList(); 

                foreach (var topPizza in subset)
                    result.Add(topPizza);

                return result;
            }
        }

        // List<PizzaDataRaw> list = new List<PizzaDataRaw>();
    }

    //
    // Almost a dummy class for the pizza toppings bitmap. It's
    // required only to introduce the PizzaEqualityComparer
    public class PizzaSignature 
    {
        public List<char> data { get; set; }
        public PizzaSignature(int n)
        {
            data = new List<char>(n);
        }
    }

    //
    // This object compares the bit arrays of the variable length
    // by padding them with zeros
    public class PizzaEqualityComparer : IEqualityComparer<PizzaSignature>
    {
        public bool Equals(PizzaSignature k1, PizzaSignature k2)
        {
            string keyX = new string(k1.data.ToArray());
            string keyY = new string(k2.data.ToArray());
            int maxLength = Math.Max(keyX.Length, keyY.Length);

            string x = keyX + new string('0', maxLength - keyX.Length);
            string y = keyY + new string('0', maxLength - keyY.Length);

            bool prefixMatch = (x == y); // just for debugging

            return prefixMatch;
        }
        public int GetHashCode(PizzaSignature x)
        {
            // plenty of collissions will make dictionary
            // use the Equals method
            return 0;
            /*int hash = 0;
            int c = 0;
            foreach(char topping in x.data)
            {
                int val = (topping == '1') ? 1 : 0;
                hash += val
            }*/
            //while(x.data.Count < )
            //return x.data.ToString().GetHashCode();
        }
    }

    //
    // The pizza data as it's stored in JSON
    sealed class PizzaDataRaw
    {
        public List<string> toppings { get; set; }
    }
}