using System;
using System.Collections;

namespace WLANWecker.Model
{
    public class Alarm
    {
        public int hour { get; set; }
        public int minute { get; set; }

        public string time
        {
            get
            {
                string minute = this.minute <= 9 ? $"0{this.minute}" : $"{this.minute}";
                return $"{hour}:{minute}";
            }
            set
            {
                time = value;
            }
        }

        public TimeSpan timeSpanClock
        {
            get
            {
                return TimeSpan.Parse(time);
            }
            set
            {
                hour = value.Hours;
                minute = value.Minutes;
            }
        }

        public string displayName { get; set; }

        public float minimumAcceleration { set; get; }

        private bool _vibrate;
        public bool vibrate {
            get { return _vibrate; }
            set
            {
                _vibrate = value;
                OnVariableChanged(nameof(vibrate));
            }
        }

        private bool _sound;
        public bool sound
        {
            get { return _sound; }
            set
            {
                _sound = value;
                OnVariableChanged(nameof(sound));
            }
        }

        private bool _repeat;
        public bool repeat
        {
            get { return _repeat; }
            set
            {
                _repeat = value;
                OnVariableChanged(nameof(repeat));
            }
        }


        public bool monday { get; set; }
        public bool tuesday { get; set; }
        public bool wednesday { get; set; }
        public bool thursday { get; set; }
        public bool friday { get; set; }
        public bool saturday { get; set; }
        public bool sunday { get; set; }

        public bool isEnabled { get; set; }
        public int id { get; set; }

        public byte idMask = 240; // <- B1111 0000 <-

        public event EventHandler<string> VariableChanged;

        protected virtual void OnVariableChanged(string name)
        {
            VariableChanged?.Invoke(this, name);
        }

        public string ToBytes()
        {
            string clockBytes = "";

            //int settingsByte1 = id << 4;

            // Bytes müsses nun in verkehrter Reihenfolge addiert werden.
            // So wird das erst gesetzte Byte am Ende ganz hinten liegen
            // B 0000 0000 -> B 0000 0001 -> B 0000 0010 -> B 0000 0100 -> etc.

            int settingsByte1 = Convert.ToInt32(repeat) << 3;
            settingsByte1 += Convert.ToInt32(vibrate) << 2;
            settingsByte1 += Convert.ToInt32(sound) << 1;
            settingsByte1 += Convert.ToInt32(isEnabled) << 0;

            
            int settingsByte2 = Convert.ToInt32(sunday) << 6;
            settingsByte2 += Convert.ToInt32(saturday) << 5;
            settingsByte2 += Convert.ToInt32(friday) << 4;
            settingsByte2 += Convert.ToInt32(thursday) << 3;
            settingsByte2 += Convert.ToInt32(wednesday) << 2;
            settingsByte2 += Convert.ToInt32(tuesday) << 1;
            settingsByte2 += Convert.ToInt32(monday) << 0;

            int hourByte = Convert.ToByte(hour);
            int minuteByte = Convert.ToByte(minute);

            int idByte = Convert.ToByte(id);

            clockBytes = $"{settingsByte1},{settingsByte2},{displayName},{hourByte},{minuteByte},{idByte}";

            Console.WriteLine("B");
            Console.WriteLine(clockBytes);

            return clockBytes;
        }

        public void FromBytes(string alarmString)
        {
            Console.WriteLine("A");
            Console.WriteLine(alarmString);
            string[] splitString = alarmString.Split(',');

            byte settingsByte1 = Convert.ToByte(splitString[0]);


            // Shifte die Bits um x nach vorner und maskiere sie mit 0b_0000_001, da nur das erste Bit zählt 2^1 = 1
            isEnabled = Convert.ToBoolean((settingsByte1 >> 0) & 1);
            sound = Convert.ToBoolean((settingsByte1 >> 1) & 1);
            vibrate = Convert.ToBoolean((settingsByte1 >> 2) & 1);
            repeat = Convert.ToBoolean((settingsByte1 >> 3) & 1);

            byte settingsByte2 = Convert.ToByte(splitString[1]);

            monday = Convert.ToBoolean((settingsByte2 >> 0) & 1);
            tuesday = Convert.ToBoolean((settingsByte2 >> 1) & 1);
            wednesday = Convert.ToBoolean((settingsByte2 >> 2) & 1);
            thursday = Convert.ToBoolean((settingsByte2 >> 3) & 1);
            friday = Convert.ToBoolean((settingsByte2 >> 4) & 1);
            saturday = Convert.ToBoolean((settingsByte2 >> 5) & 1);
            sunday = Convert.ToBoolean((settingsByte2 >> 6) & 1);


            displayName = splitString[2];

            hour = Convert.ToInt32(splitString[3]);
            minute = Convert.ToInt32(splitString[4]);

            id = Convert.ToInt32(splitString[5]);
        }

        
    }
}
