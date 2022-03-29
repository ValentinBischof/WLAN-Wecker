using System;
namespace WLANWecker.Model
{
    public class ClockItem
    {
        public string name { get; set; }
        public string displayName { get; set; }
        public string clockDescription { get; set; }

        public string ip { get; set; }

        public string IpString
        {
            get
            {
                return $"Adresse: {ip}";
            }
        }
    }
}
