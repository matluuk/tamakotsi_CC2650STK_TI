import matplotlib.pyplot as plt
import csv


# measuredData is 1, AvgData is -1 Both is 0
dataToShow = -1
g = 9.81

#Old movavg
#def movavg(arr, window_size):
#    new_arr = []
#    new_array_size = len(arr) - window_size + 1
#    for i in range(new_array_size):
#        result = 0
#        for j in range(window_size):
#            result += arr[i+j]
#        
#        new_arr.append(result / window_size)
#    return new_arr



def movavg(arr, window_size):
    new_arr = []
    sum = 0
    for i in range(len(arr)):
        sum += arr[i]
        if i >= window_size:
            sum -= arr[i - window_size]
        
        new_arr.append(sum / window_size)
    return new_arr

# Integrates simple data, over time
# returns list of integrated data
def Integrate(data, time):
    result = [0]
    for i in range(len(data) - 1):
        dt = (time[i+1] - time[i])
        result.append(result[i] + (data[i]) * dt)
    return result

def main():

    time = []
    ax = []
    ay = []
    az = []
    gx = []
    gy = []
    gz = []

    avgtime = []
    avgax = []
    avgay = []
    avgaz = []
    avggx = []
    avggy = []
    avggz = []

    
    #open file
    with open('data/liuutusOV.txt') as file:
        reader = csv.reader(file)
        for row in reader:
            if float(row[0]) >= 0:
                time.append(float(row[0]) * 0.001) #time in ms
                ax.append(float(row[1]) * g)
                ay.append(float(row[2]) * g)
                az.append((float(row[3]) + 1) * g)
                gx.append(float(row[4]))
                gy.append(float(row[5]))
                gz.append(float(row[6]))

    #calculate averages
    avgtime = movavg(time,3)
    avgax = movavg(ax, 3)
    avgay = movavg(ay, 3)
    avgaz = movavg(az, 3)
    avggx = movavg(gx, 3)
    avggy = movavg(gy, 3)
    avggz = movavg(gz, 3)


    #plots
    fig, (acc, gyro) = plt.subplots(2, figsize=(12, 8))


    #acceleration plot
    if dataToShow >= 0:
        acc.plot(time, ax, label='ax')
        acc.plot(time, ay, label='ay')
        acc.plot(time, az, label='az')
        print("nothing here")
    if dataToShow <= 0:
        acc.plot(avgtime, avgax, label='avgAx')
        acc.plot(avgtime, avgay, label='avgAy')
        acc.plot(avgtime, avgaz, label='avgAz')

    #speed
    acc.plot(avgtime, Integrate(avgax, avgtime), label='velx')
    #acc.plot(time, Integrate(avgay, avgtime), label='vely')
    #acc.plot(time, Integrate(avgaz, avgtime), label='velz')

    #position 
    acc.plot(avgtime, Integrate(Integrate(avgax, time), avgtime), label='posx')
    #acc.plot(avgtime, Integrate(Integrate(avgay, time), avgtime), label='posy')
    #acc.plot(avgtime, Integrate(Integrate(avgaz, time), avgtime), label='posz')

    acc.set_ylim([-3, 3])
    acc.set_title("kiihtyvyys acc")
    acc.set_xlabel("aika (s)")
    acc.set_ylabel("kiihtyvyys (g)\nnopeus m/s\nposition m")
    acc.legend()
    acc.grid(axis = 'y')

    #gyro plot
    if dataToShow >= 0:
        gyro.plot(time, gx, label='gx')
        gyro.plot(time, gy, label='gy')
        gyro.plot(time, gz, label='gz')
    if dataToShow <= 0:
        gyro.plot(avgtime, avggx, label='avgGx')
        gyro.plot(avgtime, avggy, label='avgGy')
        gyro.plot(avgtime, avggz, label='avgGz')
    gyro.set_ylim([-100, 100])
    gyro.set_title("asento gyro")
    gyro.set_xlabel("aika (ms)")
    gyro.set_ylabel("kulmanopeus deg/s")
    gyro.legend()
    gyro.grid(axis = 'y')
    plt.show()

if __name__ == "__main__":
    main()