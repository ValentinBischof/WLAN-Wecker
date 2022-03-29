using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using WLANWecker.ViewModel;

namespace WLANWecker.Model
{
    public class Clock
    {
        public string displayName { get; set; }
        public string ip { get; set; }
        public string currentTime { get; set; }

        public bool isLoaded { get; set; }
        
        public ObservableCollection<Alarm> alarms { get; set; }

        public bool nightShift { get; set; }
        public bool touchToWake { get; set; }

        public string ssid { get; set; }
        public string pw { get; set; }

        private int _autoLockTimesIndex = 0;
        public int autoLockTimesIndex
        {
            get
            {
                return _autoLockTimesIndex;
            }
            set
            {
                _autoLockTimesIndex = value;
            }
        }

        public string[] autoLockTimes
        {
            get
            {
                return new string[] { "30s", "1m", "2m", "3m", "4m", "5" };
            }
            private set
            {
                autoLockTimes = value;
            }
        }

        public const int maxAlarms = 255;

        public string ToBytes()
        {
            string clockBytes = "";


            return clockBytes;
        }
    }
}
