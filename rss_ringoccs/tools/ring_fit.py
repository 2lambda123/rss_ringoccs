'''
    Purpose:
        To determine the locations of a ring feature from normal optical
        depth profiles of planetary rings. This uses a Trust Region Reflective
        algorithm to fit the observed :math:`1-\\exp(-\\tau)` with a function
        to determine the exact location of a ring feature.
    Dependencies:
        #. numpy
        #. scipy
        #. spiceypy
'''
# import libraries
import numpy as np
import os, sys
from scipy.optimize import curve_fit
from scipy.special import wofz
from spiceypy import furnsh, utc2et, et2utc

class ring_fit(object):
    '''
        Purpose:
            When instantiated, to read .TAB files containing normal
            optical depth profiles reconstructed from radio occultations
            and fit a feature with user-specified location and function.
        Arguments:
            :file (*str*): string specifying the path and file containing
                            the profile to be read in, masked, and fit.
                            Note: must follow rss_ringoccs and NASA/PDS
                            naming conventions.
            :cent_guess (*float*): best initial guess for the location of
                            the feature to be fit (in kilometers)
        Keyword Arguments:
            :data_lims (*list*): 1x2 list of the lower and upper limits
                            on the data to be included in the fit (in
                            kilometers). Must bound `cent_guess`. If these
                            do not bound `cent_guess` or the input for this
                            keyword argument is not specified or is not
                            properly formatted, the script defaults to
                            a list of `[cent_guess-50,cent_guess+50]`
            :func (*str*): a string specifying the function to fit to the
                            desired feature. Options:
                                ================    ========================================================================
                                Input               Function
                                ================    ========================================================================
                                'logistic'          logistic function -- used for fitting ring, gap, and other sharp edges
                                'logistic_left'     logistic function for edge with ring on left, freespace on right
                                'logistic_right'    logistic function for edge with ring on right, freespace on left
                                'gauss'             Gaussian -- used for ringlets with a narrow base
                                'lorentz'           Lorentzian -- used for ringlets with a broad base and narrow, sharp peak
                                'voigt'             Voigt -- a compromise between Gaussian and Lorentzian profiles
                                ================    ========================================================================
                            Default is 'logistic'.
        Attributes:
            :obsid (*str*): 22 character string indicating the year, date,
                            direction, band, station, data processing,
                            and reconstruction resolution in meters
            :start_oet_utc (*str*): 28 character string indicating the UTC
                            at which the observation of the ring profile began
            :rho (*np.ndarray*): masked ring intercept radii used in fit
            :pow (*np.ndarray*): masked "power" (:math:`$1-\\exp(-\\tau)$`)
                            used in  fit
            :fit (*np.ndarray*): best fit to masked profile
            :cent_oet_utc (*str*): 28 character string indicating the UTC
                            time at which the observation of the center of
                            the ring feature occurred
            :cent_oet_spm (*float*): observed event time of the ring feature
                            in seconds past midnight
            :cent_ret_spm (*float*): ring event time of the ring feature
                            in seconds past midnight
            :cent_km (*float*): ring intercept radius of the ring feature
                            in kilometers
            :cent_km_err (*float*): uncertainty in ring feature ring intercept
                            radius in kilometers
            :rho_dot_kms (*float*): ring intercept radial velocity at the
                            ring feature in kilometers per second
            :ilong_deg (*float*): inertial longitude of the ring feature in degrees
            :flag (*int*): flag indicating fit quality according to the following
                            criteria:
                                ====    ==========================================================
                                Flag    Meaning
                                ====    ==========================================================
                                0       fit is acceptable
                                1       no data available for fit
                                2       feature not detected ( ABS(A-N) < STDV(DATA-FIT) )
                                3       feature location not well-constrained ( uncertainty > 1 km )
                                ====    ==========================================================
        Notes:
            #.  The software assumes the files are named and stored following
                the output conventions and heirarchy of `rss_ringoccs` and the
                NASA Planetary Data System. This script will not be able to
                read in data sorted or stored in any other hierarchy.
            #.  If fewer data than the number of fit parameters are available
                from the input reconstructed optical depth profile, then the
                fit cannot be performed and all attributes related to the masked
                profile data and fit thereto are set to zero.
            #.  If the desired feature is not detected above the noise but
                sufficient data exist for calculating the best fit, a warning and
                a flag are raised, but the fit is still conducted.
    '''
    def __init__(self,file,cent_guess,data_lims=None,func='logistic'):

        # function options
        funcs = {'logistic':self.__logistic,'gauss':self.__gauss,
                 'lorentz':self.__lorentz,'voigt':self.__voigt}

        # check to make sure input file exists
        if not os.path.exists(file) :
            sys.stdout.write("\r")
            sys.stdout.write('\n     Specified input file\n     '+file+'\n     does not exist.\n\n')
            sys.stdout.flush()
            sys.exit()

        # check input for function
        if func.split('_')[0] not in funcs.keys():
            sys.stdout.write("\r")
            sys.stdout.write('\n     Function \''+func+'\' specified to \'func\' kwarg not recognized.'
                            +'\n     Please use one of the following options:\n        '
                            +'\n        '.join(key for key in funcs.keys())+'\n\n')
            sys.stdout.flush()
            sys.exit()

        # check kwarg "data_lims" for input
        if data_lims == None or len(data_lims) != 2 or data_lims[0]>data_lims[1] :
            sys.stdout.write("\r")
            sys.stdout.write('\n     No data limits specified or limits were'
                            +'\n     not in proper format.'
                            +' Setting to default\n     limits of data_guess +/- 50 km.\n\n')
            sys.stdout.flush()
            # resort to defaults
            data_lims = [cent_guess-50,cent_guess+50]
        # if data_lims don't bound cent_guess
        elif not ((cent_guess>data_lims[0])&(cent_guess<data_lims[1])):
            sys.stdout.write("\r")
            sys.stdout.write('\n     Data guess '+str(cent_guess)+' not within data limits '
                            +','.join(dl for dl in data_lims)
                            +'\n     Setting to default limits of guess +/- 50 km.\n\n')
            sys.stdout.flush()
            # resort to defaults
            data_lims = [cent_guess-50,cent_guess+50]

        # initial parameters and parameter boundaries based on function
        # logistic
        if func == 'logistic' :
            p0 = [cent_guess,1.,-3.,0.0]
            #bounds = ([data_lims[0],0,-10,-0.1],[data_lims[1],2,-2,0.1])
            bounds = ([data_lims[0],-np.inf,-np.inf,-0.1],[data_lims[1],np.inf,np.inf,0.1])
        # gaussian or lorentzian profile
        elif func == 'gauss' or func == 'lorentz' :
            p0  = [cent_guess,1.,1.,0.0]
            bounds = ([data_lims[0],0,-np.inf,-0.1],[data_lims[1],np.inf,np.inf,0.1])
        # voigt profile
        elif func == 'voigt' :
            p0  = [cent_guess,1.,1.,1.,0.0]
            bounds = ([data_lims[0],0,-np.inf,-np.inf,-0.1],[data_lims[1],np.inf,np.inf,np.inf,0.1])

        # get occultation info from filepath, assuming PDS and rss_ringoccs heirarchy and naming
        rev_info = file.split('/')[-2].split('_')
        rev = rev_info[0][3:6]
        profdir = rev_info[0][6:]
        year  = rev_info[2]
        doy = int(rev_info[3])
        band = rev_info[4][0]
        dsn = 'DSS-'+rev_info[4][1:]
        res = file.split('/')[-1].split('_')[6][:-1]
        if 'KM' in res:
            res = res[:2]+'000'
        elif 'M' in res:
            res = '0'+res[:4]
        # determine source of profile by length of output file name
        # "TC" for Team Cassini 2017-2019 (TC file contains date+serial)
        # "EM" for Essam Marouf 2018 from NASA/PDS
        if len(file.split('/')[-1]) >= 40 :
            src = 'TC'
        else:
            src = 'EM'

        # observation ID
        self.obsid = 'RSS_'+rev+profdir[-1]+'_'+band+dsn.replace('DSS-','')+'_'+src+'_'+res.strip()+'m'


        # load in data
        rho,lamb,tau,phase,oet,ret = np.loadtxt(file,delimiter=',',usecols=(0,3,6,7,9,10)).T

        # acquire UTC of the day of the observation
        self.start_oet_utc = self.revinfo2utc(year,doy,np.min(oet))

        # mask to data limits
        mask = [(rho>data_lims[0])&(rho<data_lims[1])]

        # store masked data as attributes
        self.rho = rho[mask]
        self.tau = tau[mask]
        self.pow = 1. - np.exp(-tau[mask])
        self.phase = phase[mask]
        self.oet = oet[mask]
        self.ret = ret[mask]

        # fit flag -- default as well-fit
        self.flag = int(0)

        # check mask to make sure sufficient data are present in desired radius range
        if len(self.pow) > 5 :

            # force "normalize" if freespace is offset from zero
            # to keep profile values consistent with parameter bounds
            if np.nanmin(self.pow) > 0.0 :
                self.pow -= np.nanmin(self.pow)

            # fit using Trust Region Reflective algorithm
            par,cov = curve_fit(funcs[func],self.rho,self.pow,p0=p0,bounds=bounds,maxfev=100000)

            # store fit as attribute
            self.fit = funcs[func](self.rho,*par)
            # store results as attributes
            self.fit_parameters = par
            self.fit_covariance = cov

            # approximation of ring intercept radial velocity
            i0 = np.argmax(rho[(rho<=par[0])])  # argument of radius just below feature
            i1 = np.argmin(rho[(rho>par[0])])   # argument of radius just above feature
            rirv = (rho[i1]-rho[i0])/(ret[i1]-ret[i0]) # approximate derivative with detlas

            # fit results as attributes
            self.cent_km = par[0]  # ring feature location in km
            self.cent_km_err = np.sqrt(np.diag(cov))[0] # ring feature uncertainty in km
            self.sumsq_resid = np.sum(np.square(self.pow
                                    -funcs[func](self.rho,*par))) # summed squared residuals
            self.rms_resid = np.std(self.pow-funcs[func](self.rho,*par)) # rms residuals
            self.aon = abs(self.fit_parameters[1] - self.fit_parameters[-1])
            ### CHECK FIT RESULTS AND FLAG BAD FITS
            # flag if feature is not detected
            if self.aon < self.rms_resid and self.rms_resid < 1.0 :
                sys.stdout.write("\r")
                sys.stdout.write('\n     Ring feature not well detected for '+self.obsid
                                +'\n     Do not trust fit results.\n\n')
                sys.stdout.flush()
                # flag as not detected
                self.flag = int(2)
            # flag if fit is poorly constrained
            elif self.cent_km_err > 1. or self.rms_resid > 1.0 :
                sys.stdout.write("\r")
                sys.stdout.write('\n     Ring feature location not well constrained for '+self.obsid
                                +'\n     Do not trust fit results.\n\n')
                sys.stdout.flush()
                # flag as poorly constrained
                self.flag = int(3)

            # SPM of the observed event time of the ring feature
            self.cent_oet_spm = np.interp(par[0],self.rho,self.oet)
            # approximate uncertainty in SPM
            self.cent_oet_spm_err = abs(np.interp(par[0],self.rho+self.cent_km_err,self.oet)
                                    -np.interp(par[0],self.rho-self.cent_km_err,self.oet))/2.
            # correct DOY if in a new day
            doy += self.cent_oet_spm//86400
            # UTC of the observed event time of the ring feature
            self.cent_oet_utc = self.revinfo2utc(year,doy,self.cent_oet_spm%86400)
            # SPM of the ring event time of the ring feature
            self.cent_ret_spm = np.interp(par[0],self.rho,self.ret)
            # ring intercept radial velocity at the ring feature
            self.rho_dot_kms = rirv
            # inertial longitude of the ring feature
            self.ilong_deg = np.interp(par[0],rho[mask],lamb[mask])

        # if insufficient data present
        else:
            # inform user that insufficient data are present
            sys.stdout.write("\n\r")
            sys.stdout.write('     Insufficient data to fit feature at '+
                             '%d km for file\n     %s\n\n' % (cent_guess,file))
            sys.stdout.flush()
            # flag as lacking data
            self.flag = int(1)
            # set attributes to zero when fit is not possible
            self.fit = np.zeros(len(self.pow))
            self.fit_parameters = p0
            self.fit_covariance = np.zeros((len(p0),len(p0)))
            self.sumsq_resid = 0.0
            self.rms_resid = 0.0
            self.cent_km = 0.0
            self.cent_km_err = 0.0
            self.cent_oet_spm = 0.0
            self.cent_oet_utc = self.start_oet_utc
            self.cent_ret_spm = 0.0
            self.rho_dot_kms = 0.0
            self.ilong_deg = 0.0


    # function to convert year, day-of-year,
    # and seconds past midnight to UTC using
    # the astropy.time module
    def revinfo2utc(self,year,doy,spm):
        '''
        Purpose:
            Convert the observation time contained in the file name
            in year and day-of-year and time-of-day (in spm) in the
            input file to UTC1 format. Converts from decimal spm to
            sexigecimal to build UTC0 time string. Uses `spiceypy`
            to convert from UTC0 to UTC1.
        Arguments:
            :year (*int*, *float*, or *str*): year
            :doy (*int* or *float*): day of year
            :spm (*float*): time in seconds past midnight
        Returns:
            UTC1 time at 6 decimal precision
        '''
        # get UTC from year, doy, and spm
        # start with year:day:hour:min:sec.sec format
        y = str(int(year))
        # check if observed time overshoots into a new day
        # and if so, update DOY and spm accordingly
        if spm > 86400 :
            doy += 1
            spm -= 86400
        # day of year as 3 char string
        d = str(int(doy))
        if len(d) == 1:
            d = '00'+d
        elif len(d) == 2:
            d = '0'+d
        # hour as 2 char string
        h = str(int(spm//3600))
        if len(h) == 1:
            h = '0'+h
        # min as 2 char string
        m = str(int((spm%3600)//60))
        if len(m) == 1:
            m = '0'+m
        # secs as 11 char string
        s = str(round((spm%3600)%60,8))
        if len(s.split('.')[0]) == 1 :
            s = '0'+s

        # utc format in calender format: eg "1986 APR 12 16:31:09.814"
        furnsh('../kernels/naif/CASSINI/kernels/lsk/naif0012.tls')
        utc0 = year+'-'+d+'T'+h+':'+m+':'+s
        et = utc2et(utc0)
        utc1 = et2utc(et,'C', 6)
        return utc1

    # logistic function
    def __logistic(self,x,x0,L,k,c):
        return L/(1.+np.exp(-k*(x-x0)))+c

    # gaussian function
    def __gauss(self,x,x0,L,k,c):
        norm = L*np.sqrt(k/np.pi)
        return norm*np.exp(-k*(x-x0)**2)+c

    # lorentzian function
    def __lorentz(self,x,x0,L,k,c):
        norm = L*np.sqrt(k)
        return norm/(np.pi*(x-x0)**2+k)+c

    # voigt function
    def __voigt(self,x,x0,L,k1,k2,c):
        z = ((x-x0)+1j*k1)/k2
        w = wofz(z)
        norm = L/(k2*np.sqrt(np.pi))
        return norm*np.real(w)+c
