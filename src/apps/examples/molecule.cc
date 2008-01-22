

/// \file molecule.cc
/// \brief Simple management of molecular geometry


#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>

static std::string lowercase(const std::string& s) {
    std::string r(s);
    for (unsigned int i=0; i<r.size(); i++) r[i] = tolower(r[i]);
    return r;
}

struct AtomicData {
    // !!! The order of declaration here must match the order in the initializer !!!
    
    // Nuclear info from L. Visscher and K.G. Dyall, Dirac-Fock
    // atomic electronic structure calculations using different
    // nuclear charge distributions, Atom. Data Nucl. Data Tabl., 67,
    // (1997), 207.
    //
    // http://dirac.chem.sdu.dk/doc/FiniteNuclei/FiniteNuclei.shtml
    const char* const symbol;
    const char* const symbol_lowercase;
    const int atomic_number;
    const int isotope_number;
    const double nuclear_radius;     //< Radius of the nucleus for the finite nucleus models (in atomic units).
    const double nuclear_half_charge_radius; //< Half charge radius in the Fermi Model (in atomic units).
    const double nuclear_gaussian_exponent; //< Exponential parameter in the Gaussian Model (in atomic units).

    /// Covalent radii stolen without shame from NWChem
    const double covalent_radius;
};

static const int NUMBER_OF_ATOMS_IN_TABLE = 109;
static const AtomicData atomic_data[NUMBER_OF_ATOMS_IN_TABLE] = {
    {"H",   "h",    1  ,  1   ,  2.6569547399e-05  , 1.32234e-05   ,2.1248239171E+09, 0.30   },
    {"He",  "he",   2  ,  4   ,  3.5849373401e-05  , 2.63172e-05   ,1.1671538870E+09, 1.22   },
    {"Li",  "li",   3  ,  7   ,  4.0992133976e-05  , 2.34051e-05   ,8.9266848806E+08, 1.23   },
    {"Be",  "be",   4  ,  9   ,  4.3632829651e-05  , 3.03356e-05   ,7.8788802914E+08, 0.89   },
    {"B",   "b",    5  ,  11  ,  4.5906118608e-05  , 3.54894e-05   ,7.1178709563E+08, 0.88   },
    {"C",   "c",    6  ,  12  ,  4.6940079496e-05  , 3.76762e-05   ,6.8077502929E+08, 0.77   },
    {"N",   "n",    7  ,  14  ,  4.8847128967e-05  , 4.15204e-05   ,6.2865615725E+08, 0.70   },
    {"O",   "o",    8  ,  16  ,  5.0580178957e-05  , 4.48457e-05   ,5.8631436655E+08, 0.66   },
    {"F",   "f",    9  ,  19  ,  5.2927138943e-05  , 4.91529e-05   ,5.3546911034E+08, 0.58   },
    {"Ne",  "ne",  10  ,  20  ,  5.3654104231e-05  , 5.04494e-05   ,5.2105715255E+08, 1.60   },
    {"Na",  "na",  11  ,  23  ,  5.5699159416e-05  , 5.40173e-05   ,4.8349721509E+08, 1.66   },
    {"Mg",  "mg",  12  ,  24  ,  5.6341070732e-05  , 5.51157e-05   ,4.7254270882E+08, 1.36   },
    {"Al",  "al",  13  ,  27  ,  5.8165765928e-05  , 5.81891e-05   ,4.4335984491E+08, 1.25   },
    {"Si",  "si",  14  ,  28  ,  5.8743802504e-05  , 5.91490e-05   ,4.3467748823E+08, 1.17   },
    {"P",   "p",   15  ,  31  ,  6.0399312923e-05  , 6.18655e-05   ,4.1117553148E+08, 1.10   },
    {"S",   "s",   16  ,  32  ,  6.0927308666e-05  , 6.27224e-05   ,4.0407992047E+08, 1.04   },
    {"Cl",  "cl",  17  ,  35  ,  6.2448101115e-05  , 6.51676e-05   ,3.8463852873E+08, 0.99   },
    {"Ar",  "ar",  18  ,  40  ,  6.4800211825e-05  , 6.88887e-05   ,3.5722217300E+08, 1.91   },
    {"K",   "k",   19  ,  39  ,  6.4346167051e-05  , 6.81757e-05   ,3.6228128110E+08, 2.03   },
    {"Ca",  "ca",  20  ,  40  ,  6.4800211825e-05  , 6.88887e-05   ,3.5722217300E+08, 1.74   },
    {"Sc",  "sc",  21  ,  45  ,  6.6963627201e-05  , 7.22548e-05   ,3.3451324570E+08, 1.44   },
    {"Ti",  "ti",  22  ,  48  ,  6.8185577480e-05  , 7.41350e-05   ,3.2263108827E+08, 1.32   },
    {"V",   "v",   23  ,  51  ,  6.9357616830e-05  , 7.59254e-05   ,3.1181925878E+08, 1.22   },
    {"Cr",  "cr",  24  ,  52  ,  6.9738057221e-05  , 7.65040e-05   ,3.0842641793E+08, 1.19   },
    {"Mn",  "mn",  25  ,  55  ,  7.0850896638e-05  , 7.81897e-05   ,2.9881373610E+08, 1.17   },
    {"Fe",  "fe",  26  ,  56  ,  7.1212829817e-05  , 7.87358e-05   ,2.9578406371E+08, 1.165  },
    {"Co",  "co",  27  ,  59  ,  7.2273420879e-05  , 8.03303e-05   ,2.8716667270E+08, 1.16   },
    {"Ni",  "ni",  28  ,  58  ,  7.1923970253e-05  , 7.98058e-05   ,2.8996391416E+08, 1.15   },
    {"Cu",  "cu",  29  ,  63  ,  7.3633018675e-05  , 8.23625e-05   ,2.7665979354E+08, 1.17   },
    {"Zn",  "zn",  30  ,  64  ,  7.3963875193e-05  , 8.28551e-05   ,2.7419021043E+08, 1.25   },
    {"Ga",  "ga",  31  ,  69  ,  7.5568424848e-05  , 8.52341e-05   ,2.6267002737E+08, 1.25   },
    {"Ge",  "ge",  32  ,  74  ,  7.7097216161e-05  , 8.74862e-05   ,2.5235613399E+08, 1.22   },
    {"As",  "as",  33  ,  75  ,  7.7394645153e-05  , 8.79228e-05   ,2.5042024280E+08, 1.21   },
    {"Se",  "se",  34  ,  80  ,  7.8843427408e-05  , 9.00427e-05   ,2.4130163719E+08, 1.17   },
    {"Br",  "br",  35  ,  79  ,  7.8558604038e-05  , 8.96268e-05   ,2.4305454351E+08, 1.14   },
    {"Kr",  "kr",  36  ,  84  ,  7.9959560033e-05  , 9.16684e-05   ,2.3461213272E+08, 1.98   },
    {"Rb",  "rb",  37  ,  85  ,  8.0233033713e-05  , 9.20658e-05   ,2.3301551109E+08, 2.22   },
    {"Sr",  "sr",  38  ,  88  ,  8.1040799081e-05  , 9.32375e-05   ,2.2839354730E+08, 1.92   },
    {"Y",   "y",   39  ,  89  ,  8.1305968993e-05  , 9.36215e-05   ,2.2690621893E+08, 1.62   },
    {"Zr",  "zr",  40  ,  90  ,  8.1569159980e-05  , 9.40022e-05   ,2.2544431039E+08, 1.45   },
    {"Nb",  "nb",  41  ,  93  ,  8.2347219223e-05  , 9.51261e-05   ,2.2120420724E+08, 1.34   },
    {"Mo",  "mo",  42  ,  98  ,  8.3607614434e-05  , 9.69412e-05   ,2.1458511597E+08, 1.29   },
    {"Tc",  "tc",  43  ,  98  ,  8.3607614434e-05  , 9.69412e-05   ,2.1458511597E+08, 1.27   },
    {"Ru",  "ru",  44  , 102  ,  8.4585397905e-05  , 9.83448e-05   ,2.0965270287E+08, 1.24   },
    {"Rh",  "rh",  45  , 103  ,  8.4825835954e-05  , 9.86893e-05   ,2.0846586999E+08, 1.25   },
    {"Pd",  "pd",  46  , 106  ,  8.5537941156e-05  , 9.97084e-05   ,2.0500935221E+08, 1.28   },
    {"Ag",  "ag",  47  , 107  ,  8.5772320442e-05  , 1.00043e-04   ,2.0389047621E+08, 1.34   },
    {"Cd",  "cd",  48  , 114  ,  8.7373430179e-05  , 1.02327e-04   ,1.9648639618E+08, 1.41   },
    {"In",  "in",  49  , 115  ,  8.7596760865e-05  , 1.02644e-04   ,1.9548577691E+08, 1.50   },
    {"Sn",  "sn",  50  , 120  ,  8.8694413774e-05  , 1.04204e-04   ,1.9067718154E+08, 1.40   },
    {"Sb",  "sb",  51  , 121  ,  8.8910267995e-05  , 1.04510e-04   ,1.8975246242E+08, 1.41   },
    {"Te",  "te",  52  , 130  ,  9.0801452955e-05  , 1.07185e-04   ,1.8193056289E+08, 1.37   },
    {"I",   "i",   53  , 127  ,  9.0181040290e-05  , 1.06309e-04   ,1.8444240538E+08, 1.33   },
    {"Xe",  "xe",  54  , 132  ,  9.1209776425e-05  , 1.07762e-04   ,1.8030529331E+08, 2.09   },
    {"Cs",  "cs",  55  , 133  ,  9.1412392742e-05  , 1.08047e-04   ,1.7950688281E+08, 2.35   },
    {"Ba",  "ba",  56  , 138  ,  9.2410525664e-05  , 1.09453e-04   ,1.7565009043E+08, 1.98   },
    {"La",  "la",  57  , 139  ,  9.2607247118e-05  , 1.09730e-04   ,1.7490463170E+08, 1.69   },
    {"Ce",  "ce",  58  , 140  ,  9.2803027311e-05  , 1.10006e-04   ,1.7416744147E+08, 1.65   },
    {"Pr",  "pr",  59  , 141  ,  9.2997877424e-05  , 1.10279e-04   ,1.7343837120E+08, 1.65   },
    {"Nd",  "nd",  60  , 144  ,  9.3576955934e-05  , 1.11093e-04   ,1.7129844956E+08, 1.64   },
    {"Pm",  "pm",  61  , 145  ,  9.3768193375e-05  , 1.11361e-04   ,1.7060044589E+08, 1.65   },
    {"Sm",  "sm",  62  , 152  ,  9.5082839751e-05  , 1.13204e-04   ,1.6591550422E+08, 1.66   },
    {"Eu",  "eu",  63  , 153  ,  9.5267329183e-05  , 1.13462e-04   ,1.6527352089E+08, 1.65   },
    {"Gd",  "gd",  64  , 158  ,  9.6177915369e-05  , 1.14735e-04   ,1.6215880671E+08, 1.61   },
    {"Tb",  "tb",  65  , 159  ,  9.6357719009e-05  , 1.14986e-04   ,1.6155419421E+08, 1.59   },
    {"Dy",  "dy",  66  , 162  ,  9.6892647152e-05  , 1.15733e-04   ,1.5977529080E+08, 1.59   },
    {"Ho",  "ho",  67  , 162  ,  9.6892647152e-05  , 1.15733e-04   ,1.5977529080E+08, 1.58   },
    {"Er",  "er",  68  , 168  ,  9.7943009317e-05  , 1.17198e-04   ,1.5636673634E+08, 1.57   },
    {"Tm",  "tm",  69  , 169  ,  9.8115626740e-05  , 1.17438e-04   ,1.5581702004E+08, 1.56   },
    {"Yb",  "yb",  70  , 174  ,  9.8968651305e-05  , 1.18625e-04   ,1.5314257850E+08, 1.56   },
    {"Lu",  "lu",  71  , 175  ,  9.9137288835e-05  , 1.18859e-04   ,1.5262201512E+08, 1.56   },
    {"Hf",  "hf",  72  , 180  ,  9.9970978172e-05  , 1.20018e-04   ,1.5008710340E+08, 1.44   },
    {"Ta",  "ta",  73  , 181  ,  1.0013585755e-04  , 1.20246e-04   ,1.4959325643E+08, 1.34   },
    {"W",   "w",   74  , 184  ,  1.0062688070e-04  , 1.20928e-04   ,1.4813689532E+08, 1.30   },
    {"Re",  "re",  75  , 187  ,  1.0111259523e-04  , 1.21601e-04   ,1.4671710337E+08, 1.28   },
    {"Os",  "os",  76  , 192  ,  1.0191070333e-04  , 1.22706e-04   ,1.4442808782E+08, 1.26   },
    {"Ir",  "ir",  77  , 193  ,  1.0206865731e-04  , 1.22925e-04   ,1.4398142103E+08, 1.26   },
    {"Pt",  "pt",  78  , 195  ,  1.0238293593e-04  , 1.23360e-04   ,1.4309883584E+08, 1.29   },
    {"Au",  "au",  79  , 197  ,  1.0269507292e-04  , 1.23792e-04   ,1.4223027307E+08, 1.34   },
    {"Hg",  "hg",  80  , 202  ,  1.0346628039e-04  , 1.24857e-04   ,1.4011788914E+08, 1.44   },
    {"Tl",  "tl",  81  , 205  ,  1.0392291259e-04  , 1.25488e-04   ,1.3888925203E+08, 1.55   },
    {"Pb",  "pb",  82  , 208  ,  1.0437511130e-04  , 1.26112e-04   ,1.3768840081E+08, 1.54   },
    {"Bi",  "bi",  83  , 209  ,  1.0452487744e-04  , 1.26318e-04   ,1.3729411599E+08, 1.52   },
    {"Po",  "po",  84  , 209  ,  1.0452487744e-04  , 1.26318e-04   ,1.3729411599E+08, 1.53   },
    {"At",  "at",  85  , 210  ,  1.0467416660e-04  , 1.26524e-04   ,1.3690277000E+08, 1.50   },
    {"Rn",  "rn",  86  , 222  ,  1.0642976299e-04  , 1.28942e-04   ,1.3242350205E+08, 2.20   },
    {"Fr",  "fr",  87  , 223  ,  1.0657317899e-04  , 1.29139e-04   ,1.3206733609E+08, 3.24   },
    {"Ra",  "ra",  88  , 226  ,  1.0700087100e-04  , 1.29727e-04   ,1.3101367628E+08, 2.68   },
    {"Ac",  "ac",  89  , 227  ,  1.0714259349e-04  , 1.29922e-04   ,1.3066730974E+08, 2.25   },
    {"Th",  "th",  90  , 232  ,  1.0784503195e-04  , 1.30887e-04   ,1.2897067480E+08, 2.16   },
    {"Pa",  "pa",  91  , 231  ,  1.0770535752e-04  , 1.30695e-04   ,1.2930539512E+08, 1.93   },
    {"U",   "u",   92  , 238  ,  1.0867476102e-04  , 1.32026e-04   ,1.2700881714E+08, 3.00   },
    {"Np",  "np",  93  , 237  ,  1.0853744903e-04  , 1.31838e-04   ,1.2733038109E+08, 1.57   },
    {"Pu",  "pu",  94  , 244  ,  1.0949065967e-04  , 1.33145e-04   ,1.2512299012E+08, 1.81   },
    {"Am",  "am",  95  , 243  ,  1.0935561268e-04  , 1.32960e-04   ,1.2543221826E+08, 2.21   },
    {"Cm",  "cm",  96  , 247  ,  1.0989359973e-04  , 1.33697e-04   ,1.2420711085E+08, 1.43   },
    {"Bk",  "bk",  97  , 247  ,  1.0989359973e-04  , 1.33697e-04   ,1.2420711085E+08, 1.42   },
    {"Cf",  "cf",  98  , 251  ,  1.1042580946e-04  , 1.34426e-04   ,1.2301273547E+08, 1.40   },
    {"Es",  "es",  99  , 252  ,  1.1055797721e-04  , 1.34607e-04   ,1.2271879740E+08, 1.39   },
    {"Fm",  "fm",  100 , 257  ,  1.1121362374e-04  , 1.35504e-04   ,1.2127611477E+08, 1.38   },
    {"Md",  "md",  101 , 258  ,  1.1134373034e-04  , 1.35682e-04   ,1.2099285491E+08, 1.37   },
    {"No",  "no",  102 , 259  ,  1.1147350119e-04  , 1.35859e-04   ,1.2071131346E+08, 1.36   },
    {"Lr",  "lr",  103 , 262  ,  1.1186082063e-04  , 1.36389e-04   ,1.1987683191E+08, 1.34   },
    {"Db",  "db",  104 , 261  ,  1.1173204420e-04  , 1.36213e-04   ,1.2015331850E+08, 1.40   },
    {"Jl",  "jl",  105 , 262  ,  1.1186082063e-04  , 1.36389e-04   ,1.1987683191E+08, 1.40   },
    {"Rf",  "rf",  106 , 263  ,  1.1198926979e-04  , 1.36565e-04   ,1.1960199758E+08, 1.40   },
    {"Bh",  "bh",  107 , 262  ,  1.1186082063e-04  , 1.36389e-04   ,1.1987683191E+08, 1.40   },
    {"Hn",  "hn",  108 , 265  ,  1.1224519460e-04  , 1.36914e-04   ,1.1905722195E+08, 1.40   },
    {"Mt",  "mt",  109 , 266  ,  1.1237267433e-04  , 1.37088e-04   ,1.1878724932E+08, 1.40   } };
    
int symbol_to_atomic_number(const std::string& symbol) {
    std::string tlow = lowercase(symbol);
    for (int i=0; i<NUMBER_OF_ATOMS_IN_TABLE; i++) {
        if (tlow.compare(atomic_data[i].symbol_lowercase) == 0) return i;
    }
    throw "unknown atom";
}


class Atom {
public:
    double x, y, z;             //< Coordinates in atomic units
    int atn;                    //< Atomic number

    Atom(double x, double y, double z, int atn)
        : x(x), y(y), z(z), atn(atn)
    {}

    template <typename Archive>
    void serialize(Archive& ar) {ar & x & y & z & atn;}
};

std::ostream& operator<<(std::ostream& s, const Atom& atom) {
    s << "Atom(" << atom.x << ", " << atom.y << ", " << atom.z << ", " << atom.atn << ")";
    return s;
}

class Molecule {
private:
    std::vector<Atom> atoms;
    
public:    
    /// Makes a molecule with zero atoms
    Molecule() : atoms() {};
    
    /// Read coordinates from a file
    
    /// Scans the file for the first geometry block in the format
    /// \code
    ///    geometry
    ///       tag x y z
    ///       ...
    ///    end
    /// \endcode
    /// The charge \c q is inferred from the tag which is
    /// assumed to be the standard symbol for an element.
    /// Same as the simplest NWChem format.
    ///
    /// This code is just for the examples ... don't trust it!
    Molecule(const std::string& filename) {
        std::ifstream f(filename.c_str());
        std::string s;
        while (std::getline(f,s)) {
            std::string::size_type loc = s.find("geometry", 0);
            if(loc != std::string::npos) goto found_it;
        }
        throw "No geometry found in the input file";
    found_it:

        while (std::getline(f,s)) {
            std::string::size_type loc = s.find("end", 0);
            if(loc != std::string::npos) goto finished;
            std::istringstream ss(s);
            double xx, yy, zz;
            std::string tt;
            ss >> tt >> xx >> yy >> zz;
            add_atom(xx,yy,zz,symbol_to_atomic_number(tt));
        }
        throw "No end to the geometry in the input file";
    finished:
        ;
    }
    
    void add_atom(double x, double y, double z, int atn) {
        atoms.push_back(Atom(x,y,z,atn));
    }
    
    int natom() const {return atoms.size();};
    
    void set_atom_coords(unsigned int i, double x, double y, double z) {
        if (i>=atoms.size()) throw "trying to set coords of invalid atom";
        atoms[i].x = x;
        atoms[i].y = y;
        atoms[i].z = z;
    }
    
    const Atom& get_atom(unsigned int i) const {
        if (i>=atoms.size()) throw "trying to get coords of invalid atom";
        return atoms[i];
    }
    
    void print() const {
        std::cout << "      Molecule\n";
        std::cout << "      --------\n";
        std::cout.flush();
        for (int i=0; i<natom(); i++) {
            printf(" %6d   %-2s  %20.8f %20.8f %20.8f\n",
                   i, atomic_data[atoms[i].atn].symbol, atoms[i].x, atoms[i].y, atoms[i].z);
        }
    }
    
    template <typename Archive>
    void serialize(Archive& ar) {ar & atoms;}
};

int main() {
    Molecule m("fred");
    m.print();
    return 0;
}
