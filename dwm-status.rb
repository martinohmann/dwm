#!/usr/bin/env ruby
# 
# dwm-status
# 
# author: Martin Ohmann <martin@mohmann.de>
#
require 'net/http'
require 'json'

# default geo location
$geo = {
  :lat => 52.52,
  :lng => 13.41
}

# colors
$colors = {
  :white  => "\x01",
  :red    => "\x04",
  :yellow => "\x05",
  :grey   => "\x06",
  :orange => "\x07",
  :cold   => "\x08"
}

# icons glyphs from ohsnap.icons.pcf
$glyphs = {
  :wifi       => "\u20AD",
  :weather    => "\u20B1",
  :clock      => "\u20B8",
  :pacman     => "\u20BA",
  :vol_mute   => "\u20C3",
  :vol_min    => "\u20C4",
  :vol_max    => "\u20C5",
  :cpu        => "\u20CE",
  :arrow_down => "\u20DA",
  :arrow_up   => "\u20DB",
  :song_info  => "\u20EA",
  :bat_empty  => "\u20EE",
  :bat_low    => "\u20EF",
  :bat_high   => "\u20F0",
  :play       => "\uE620",
  :pause      => "\uE720"
}

class Status
  def initialize(geo = nil)
    @mutex = Mutex.new
    @weather_data = nil
    @geo = geo
  end

  def fetch_geo_info
    t = Thread.new do
      @mutex.synchronize do
        begin
          http = Net::HTTP.new('www.telize.com', 80)
          http.open_timeout = 5
          http.read_timeout = 5
          resp = http.request_get('/geoip')

          if resp.message == "OK"
            json = JSON.parse(resp.body)
            @geo = { :lat => json['latitude'], :lng => json['longitude'] }
          end
        rescue
          # @geo = nil
        end
      end
    end
    t.join

    @geo
  end

  def fetch_weather_info
    if @geo != nil
      t = Thread.new do
        @mutex.synchronize do
          begin
            http = Net::HTTP.new('api.openweathermap.org', 80)
            uri = "/data/2.5/weather?units=metric&lat=#{@geo[:lat]}&lon=#{@geo[:lng]}"
            http.open_timeout = 5
            http.read_timeout = 5
            resp = http.request_get(uri)

            if resp.message == "OK"
              json = JSON.parse(resp.body)
              temp = json['main']['temp'].to_i
              status = json['weather'][0]['id'].to_i

              # workaround: api sometimes returns 0 Kelvin
              temp = "N/A" if temp == -273

              case status
                when 200..299 then cond = "thundersorm"
                when 300..399 then cond = "drizzle"
                when 500..599 then cond = "rain"
                when 600..599 then cond = "snow"
                when 800      then cond = "clear sky"
                when 801..899 then cond = "clouds"
                else cond = json['weather'][0]['description']
              end

              @weather_data = { :temp => temp, :cond => cond }
            end
          rescue
            # @weather_data = nil
          end
        end
      end
      t.join
    end

    @weather_data
  end 

  def update_volume
    output = `amixer get Master`
    line = output.lines.last
    volume = line.sub(/.*\[([0-9]+)%\].*/, '\1').to_i

    if line.match(/\[off\]/)
      prefix = $colors[:red] + $glyphs[:vol_mute]
    else
      prefix = $colors[:grey]
      if volume == 0
        prefix += $glyphs[:vol_mute]
      elsif volume <= 40
        prefix += $glyphs[:vol_min]
      else
        prefix += $glyphs[:vol_max]
      end
    end

    @volume = sprintf("%s%s%d%%", prefix, $colors[:white], volume)
  end

  def update_cpu_temp
    temp = File.open("/sys/class/thermal/thermal_zone0/temp").first.to_i
    temp1 = File.open("/sys/class/thermal/thermal_zone1/temp").first.to_i
    temp = temp1 if temp1 > temp
    temp /=1000

    if temp >= 90
      prefix = $colors[:red] + $glyphs[:cpu] + " "
    elsif temp >= 80
      prefix = $colors[:orange] + $glyphs[:cpu] + " "
    elsif temp >= 70
      prefix = $colors[:yellow] + $glyphs[:cpu] + " "
    else
      prefix = $colors[:grey] + $glyphs[:cpu] + $colors[:white]
    end
    
    @cpu_temp = sprintf("%s%d°C", prefix, temp)
  end

  def update_battery
    output = `acpi -b`
    acpi = output.lines.first
    
    return @battery = sprintf("%s%s not present", $colors[:red], $glyphs[:bat_empty]) if acpi == nil

    state = File.open("/sys/devices/platform/smapi/BAT0/state").first.chomp
    perc = File.open("/sys/devices/platform/smapi/BAT0/remaining_percent").first.to_i

    # return @battery = nil if state == "idle" || perc == 100
  
    if perc <= 10
      prefix = $colors[:red] + $glyphs[:bat_empty] + " "
    elsif perc <= 15
      prefix = $colors[:orange] + $glyphs[:bat_low] + " "
    elsif perc <= 20
      prefix = $colors[:yellow] + $glyphs[:bat_low] + " "
    else
      prefix = $colors[:grey] + $glyphs[:bat_high] + $colors[:white]
    end

    if state == "charging"
      charge_icon = $glyphs[:arrow_up]
    elsif state == "discharging"
      charge_icon = $glyphs[:arrow_down]
    else
      charge_icon = nil
    end

    battery = sprintf("%d%%%s", perc, charge_icon)
    acpi.match(/(\d{2}:\d{2}):\d{2}/)
    battery += " #{$1}" if $1 != nil

    @battery = sprintf("%s%s", prefix, battery)
  rescue
    @battery = nil
  end

  def update_wifi
    output = `iwconfig wlp3s0`
    str = 'not connected'
    clr = $colors[:grey]
    
    if output =~ /ESSID:off\/any/
      if output =~ /Tx-Power=off/
        str = 'disabled'
      else
        output = `ifconfig enp0s25`

        if output =~ /inet/
          str = 'enp0s25'
          clr = $colors[:white]
        end
      end
    else
      output.match(/ESSID:"(.+)"/)
      str = $1
      clr = $colors[:white]
    end
    @wifi = sprintf("%s%s%s%s", $colors[:grey], $glyphs[:wifi], clr, str) 
  end

  def update_weather
    return @weather = sprintf("%s%s%s%s°C, %s", $colors[:grey], $glyphs[:weather], 
      $colors[:white], @weather_data[:temp], @weather_data[:cond]) if @weather_data != nil
    @weather = nil
  end
  
  def update_date
    time = Time.now
    @the_date = "#{$colors[:grey]}#{$glyphs[:clock]}#{$colors[:white]}#{time.strftime("%a %b %d %Y  %H:%M")}"
  end

  # get update count created by cronjob
  def update_pacman_updates
    output = `pacman -Qqu --dbpath /tmp/.localsync 2> /dev/null | wc -l`
    count = output.to_i

    return @updates = sprintf("%s%s%s%d", $colors[:grey], $glyphs[:pacman], 
      $colors[:white], count) if count > 0
    @updates = nil
  end

  # put padding behind status if trayer is running
  def update_tray_padding
    output = `xwininfo -name panel 2> /dev/null`
    if $?.success?
      if output.match(/Absolute upper-left Y:  0/)
        output.match(/Width:\s+([0-9]+)/)
        return @tray_padding = " " * ($1.to_i / 7) + " " 
      end
    end
    @tray_padding = nil
  end

  def to_s
    "#{@weather}#{@battery}#{@volume}#{@cpu_temp}#{@updates}#{@wifi}#{@the_date}#{@tray_padding}"
  end

  def run
    @start_time = Time.now

    loop do
      time_diff = (Time.now - @start_time).to_i

      # update every 30 minutes
      fetch_weather_info if time_diff % 1800 == 0

      # update every 60 minutes
      fetch_geo_info if time_diff % 3600 == 0

      update_volume
      update_date
      update_tray_padding

      # update every 10 seconds
      if time_diff % 10 == 0
        update_wifi
        update_cpu_temp
        update_battery
      end

      # update every 10 minutes
      if time_diff % 600 == 0
        update_weather
        update_pacman_updates
      end

      `xsetroot -name "#{self.to_s}"`

      sleep 1
    end
  end
end

# run
trap("SIGINT") { exit! 0 }

status = Status.new($geo)
status.run
